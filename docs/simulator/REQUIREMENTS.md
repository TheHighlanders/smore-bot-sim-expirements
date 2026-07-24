# S'mores Line — Controller Simulator: Requirements

Status: **draft for approval** (2026-07-24). Pivot from the Wokwi approach: instead
of running firmware on a stand-in MCU, we compile **only the controller logic** to
WebAssembly and drive a **bespoke browser visualizer**. All hardware/SPI is
abstracted behind unit-testable subsystem classes; the controller is the one piece
we make visible.

---

## 1. Purpose & audience

Teach students how a real machine **controller** works: each control loop it reads
sensor inputs, decides, and issues actuator commands — and it must keep track of
things the sensors can't directly see. The vehicle is a **s'mores assembly line**:
a conveyor carrying trays past three dispenser stations (graham cracker →
chocolate → marshmallow) and a **heating tunnel** that toasts the finished s'more.

Students should be able to, with **zero install** (a web page):
- Watch the line run and see the controller's decisions and internal state.
- See the controller's **estimated tray position vs. reality** (the dead-reckoning
  it does between sensors) and watch drift get corrected at each sensor.
- Poke the system (inject trays, blind a sensor, jam a gate) and see how the
  controller copes.
- Later: step the controller loop and inspect inputs/outputs each tick.

## 2. The machine

```
   entry                                                              exit
     │   ┌─ Graham ─┐      ┌─ Chocolate ┐      ┌ Marshmallow ┐   ┌ Heating tunnel ┐
 ────┴───┤  ▼ disp  ├──────┤   ▼ disp   ├──────┤   ▼ disp    ├───┤   )))  heat    ├────▶
   belt  │  ◉ sense │      │  ◉ sense   │      │  ◉ sense    │   │ ◉entry  ◉exit  │  belt
         │  ▐ gate  │      │  ▐ gate    │      │  ▐ gate     │   │  🌡 temp       │
         └──────────┘      └────────────┘      └─────────────┘   └────────────────┘
```

- **Conveyor:** one continuous belt (controller sets speed; 0 = stopped). Trays ride
  the belt and are physically held at a station by that station's closed gate.
- **Dispenser station** (×3: Graham, Chocolate, Marshmallow), each with:
  - **Light sensor** — tray present at the station's sense point (boolean).
  - **Solenoid gate** — closed holds the tray in the station; open lets it pass.
  - **Dispenser actuator** — servo/stepper/other; a pulse dispenses one unit of the
    ingredient. (Exact actuator TBD; modeled as a "dispense one unit" command.)
- **Heating tunnel** (final subsystem): a **heater** and a **temperature sensor**,
  plus **entry** and **exit** light sensors. The tray dwells in the tunnel (dwell
  set by belt speed) and the marshmallow toasts.

> Physical constants below are illustrative and tunable: belt length 1200 mm;
> sense points at 300 / 600 / 900 mm; tunnel entry 1050, exit 1200; nominal belt
> speed 100 mm/s.

## 3. Software architecture

```
        ┌────────────────────────── compiled to WASM ──────────────────────────┐
        │                                                                       │
 world  │   Controller  ── owns tray tracking + line state machine (VISUALIZED) │
 (JS /  │      │  reads inputs / writes outputs via subsystem method calls      │
 P1AM)  │      ▼                                                                │
        │   Subsystems  (stateless intent → modules; UNIT-TESTED)               │
        │   ConveyorSubsystem · StationSubsystem×3 · HeatingTunnelSubsystem     │
        │      │                                                                │
        │      ▼  HAL interface (readInput / writeOutput)                       │
        └──────┼────────────────────────────────────────────────────────────────┘
               │
      ┌────────┴─────────┐
   sim HAL            real HAL
 (visualizer I/O)   (P1AM SPI, e.g. P1AM_Sim / real P1AM)
```

- **Subsystem classes** translate a *specific, stateless intent* into a module
  action, e.g. `gate.setOpen(true)`, `dispenser.dispenseOne()`,
  `conveyor.setSpeed(mm_s)`, `tunnel.readTemperatureC()`. They hold no line logic,
  so each method is trivially unit-testable: "when I call `setGate(true)` it writes
  the expected value to the expected module output." The HAL underneath is an
  interface — the visualizer supplies a sim HAL; hardware supplies the SPI/P1AM HAL.
- **Controller class** is the only stateful logic. Each `tick(now_ms)` it:
  1. **reads** every subsystem input (sensors, temperature),
  2. **updates its world model** (tray positions via dead-reckoning; per-tray
     assembly state; per-station state machine),
  3. **decides** and **writes** outputs (belt speed, gates, dispense pulses, heater).
  The controller is what we compile to WASM and visualize.

## 4. Controller ↔ world contract (the WASM boundary)

The controller is deterministic and side-effect-free except through this contract.
The visualizer (or the P1AM HAL) owns physics/hardware; the controller owns logic.
Marshalled as flat structs in WASM linear memory (JS writes inputs → `tick` → JS
reads outputs); exact encoding finalized at implementation.

**Inputs (world → controller), each tick**

| Field | Type | Meaning |
|-------|------|---------|
| `now_ms` | u32 | monotonic sim/board time |
| `sense[0..2]` | bool | light sensor at Graham / Chocolate / Marshmallow station |
| `tunnel_entry` | bool | tray at tunnel entry |
| `tunnel_exit` | bool | tray at tunnel exit |
| `tunnel_temp_c` | f32 | tunnel temperature |
| `run` | bool | operator run/stop (E-stop clear) |
| `dispense_confirm[0..2]` | bool | *(proposed, optional)* a unit actually left the chute — lets the controller close the loop on contents (see §5.1) |

**Outputs (controller → world), each tick**

| Field | Type | Meaning |
|-------|------|---------|
| `belt_speed` | f32 | commanded belt speed (mm/s; 0 = stop) |
| `gate_open[0..2]` | bool | solenoid gate per dispenser station |
| `dispense[0..2]` | bool | rising edge = dispense one unit at that station |
| `heater_on` | bool | tunnel heater (or `heater_duty` f32 later) |

**Exposed controller state (read-only, for visualization/inspection)**

| Field | Meaning |
|-------|---------|
| `trays[]` | per tracked tray: `id`, `est_pos_mm`, `stage` (which ingredients placed), `status` (moving / held@station / toasting / done / lost) |
| `belt_pos_mm` | integrated belt travel |
| `decisions[]` | human-readable log lines emitted this tick |

This same struct pair is the seam for **unit tests** (feed inputs, assert outputs
and exposed state) and for the **visualizer** (feed simulated sensors, render
outputs + state).

## 5. Tray tracking (the headline logic)

- The belt is continuous; **between sensors no device sees the tray**, so the
  controller integrates position: `est_pos += belt_speed * dt`.
- When a station's light sensor **rises**, the controller **snaps** the matching
  tracked tray to that sensor's known position (correction), bounding drift.
- The controller must handle a tray it has never sensed yet (enters the belt,
  first seen at Graham) and a tray that has left all sensors (between stations,
  past the tunnel exit).
- **Multiple trays:** the model tracks a list; association logic decides which
  tracked tray a sensor edge belongs to (nearest expected). The demo runs one
  tray, but data structures and association must not assume a single tray.
- **Fault awareness (stretch):** if an expected sensor edge never arrives (jam) or
  an unexpected one does (mis-track), flag the tray `lost` and surface it.

### 5.1 Contents tracking (same idea, applied to ingredients)

Tray *contents* mirror tray *position*: there is **actual** contents (ground
truth, owned by the world/hardware) and the controller's **believed** contents
(inferred only from the dispense commands it issued). A real dispenser fault —
a misfire (0 units) or a double-drop (2 units) — changes the actual contents;
with no feedback the controller stays wrong (open-loop), exactly as dead-reckoned
position drifts when no sensor sees the tray.

- **Baseline (open-loop, no contract change):** the controller counts its own
  dispense pulses as believed contents; the divergence from actual is the
  teachable failure. *(Implemented in the mockup.)*
- **Optional feedback (`dispense_confirm[k]`):** a chute drop-sensor lets the
  controller verify each dispense and **retry** a miss / flag an over-fill — the
  realistic closed-loop upgrade. This is the one contract addition the contents
  feature needs; open-loop needs none.

The controller must **never** read actual contents directly — only its commands
and (optionally) the confirm sensor.

## 6. Per-station operation (controller state machine, per tray)

`APPROACHING → ARRIVED (sensor rise, close gate) → DISPENSING (pulse actuator) →
DWELL (ingredient settle) → RELEASE (open gate) → DEPARTED`. Tunnel variant:
`ENTER → TOASTING (heater on, dwell by belt speed / until temp·time target) →
EXIT`. Gate default is **open** (belt flows) and closes only to hold a tray for its
operation.

## 7. Functional requirements

- **FR-1** Controller runs a fixed-rate loop; each tick reads all inputs, updates
  state, writes all outputs. No blocking/sleeping inside a tick.
- **FR-2** Controller assembles in order: graham → chocolate → marshmallow, one
  dispense per ingredient per tray, then toast in the tunnel.
- **FR-3** A station holds its tray (gate closed) only long enough to dispense +
  settle, then releases; it must not release before dispensing.
- **FR-4** Controller estimates each tray's position between sensors and corrects
  on sensor edges (§5).
- **FR-5** Controller never commands two trays into the same occupied station
  (gate/queue management) — relevant once multiple trays run.
- **FR-6** Tunnel toasts each tray for a target dwell/temperature before it exits.
- **FR-7** On `run=false` the controller safely stops the belt and holds state;
  resuming continues correctly.
- **FR-8** All subsystem methods are pure "intent → module output" with no line
  logic, and are unit-testable against a mock HAL.

## 8. Simulator / visualizer requirements

- **VR-1** Single self-contained web page; **no install, no accounts, works
  offline** once loaded. (Bespoke — not Wokwi.)
- **VR-2** Loads the controller as a **WASM module** and steps it against a JS
  "world" that models belt/tray physics and generates sensor inputs from *actual*
  tray positions. For this draft, a **JS mock controller** stands in behind the
  same interface (WASM drops in later unchanged).
- **VR-3** Animated line view: moving belt, trays with a growing ingredient stack,
  gates opening/closing, dispense drops, tunnel heat glow, marshmallow browning.
- **VR-4** **Estimated-vs-actual overlay:** show the controller's estimated tray
  position (ghost) against the real tray, plus a live drift value — the core
  teaching moment.
- **VR-5** Controller inspector: tick counter, decision log, and a live table of
  the input/output contract (§4).
- **VR-6** Controls: play / pause / **single-step the loop**, sim-speed slider,
  **inject tray**, and fault injection (**blind a sensor**, **jam a gate**).
- **VR-7** Theme-aware (light/dark), responsive, no horizontal page scroll.
- **VR-8** The visual layer reads only the exposed contract (§4) — swapping the
  mock for the real WASM controller requires no visualizer changes.
- **VR-9** A **signal-history timeline** (AdvantageScope-style): every contract
  input/output over time in one image — analog traces (belt, temp) + digital
  lanes (sensors, gates, heater, per-station dispense pulses) with a time cursor.
- **VR-10** Per-tray **contents display**: actual vs believed per ingredient
  (ok / missing / double), so dispenser faults read at a glance (§5.1).

## 9. Testing strategy

- **Subsystem unit tests (host, fast):** assert each subsystem method issues the
  correct module intent against a mock HAL (e.g. `setGate(true)` → output bit set).
- **Controller logic tests (host, fast):** drive the controller with scripted
  input sequences (sensor edges over time) and assert outputs + exposed state —
  e.g. "graham dispensed exactly once before the gate opens", "estimated position
  within tolerance", "tray marked done after tunnel exit". Same struct contract as
  §4, no simulator needed.
- **Visualizer integration:** scripted scenarios drive the WASM controller and
  assert serial/decision output and final tray state (analogous to the Wokwi
  scenarios, but fully local).

## 10. Non-functional

- Deterministic given the same input sequence (for reproducible tests + demos).
- Controller has no dynamic allocation in the loop where avoidable (embedded-
  friendly), and no dependence on the host beyond the §4 contract.
- Runs on the real P1AM by swapping the sim HAL for the P1AM SPI HAL (reusing the
  earlier `P1AM_Sim`/real-P1AM work), with the *same* controller + subsystems.

## 11. Out of scope / assumptions / open questions

- Exact dispenser actuator (servo vs stepper vs solenoid) is abstracted to
  "dispense one unit" — revisit when hardware is chosen.
- Toast model (time-at-temperature vs fixed dwell) is a placeholder in the mock.
- Recipe is fixed (graham→choc→marshmallow); a configurable recipe is a later idea.
- Open: belt speed — fixed nominal vs. controller-varied? (Draft: controller sets a
  nominal speed and stops for safety; per-tray speed changes are out of scope now.)
- Open: does a tray enter only when the operator injects one, or continuously?
  (Draft: on demand via "inject tray".)
