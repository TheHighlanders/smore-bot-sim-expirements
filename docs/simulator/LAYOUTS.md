# S'mores Line — Configurable Mechanism Layouts: Requirements

Status: **draft for approval** (2026-07-24). Extends
[REQUIREMENTS.md](REQUIREMENTS.md); where this doc says "§N" without a file it
means a section of that document, and "§L-N" means a section here.

> **See also** [HAL.md](HAL.md): the hardware-abstraction boundary is formalized
> at the **subsystem** level, and the layout gains an **I/O binding** (module /
> slot / channel per signal) so a real-hardware implementation can be generated
> and validated. HAL.md §H-6 supersedes the contract shape discussed in §L-5
> below for anything hardware-facing.

This spec turns the line from a **fixed** topology (3 dispensers → tunnel, hard-
coded `[0..2]` arrays in the §4 contract) into a **data-driven, end-user-
configurable** one. The immediate motivating change — a **second graham
dispenser after the tunnel** to cap the s'more into a sandwich — is the first
layout that the fixed contract *cannot* express, so it drives the generalization.

---

## L-1. Motivation & goals

Today the machine is welded shut: exactly three dispensers at 300/600/900 mm and
one tunnel, with the controller contract (§4) carrying fixed `sense[3]`,
`gate_open[3]`, `dispense[3]`, `dispense_confirm[3]`. Two things break that:

1. **A capper after the tunnel.** A real s'more is a *sandwich*: graham →
   chocolate → marshmallow → **toast** → **top graham**. The top graham is a
   dispenser that must fire *after* the tunnel, so the recipe has a step past the
   tunnel and a 4th dispenser. `dispense[3]` has no room for it, and the fixed
   "3 dispensers then tunnel" ordering can't place a station downstream of the
   tunnel.
2. **Mechanism variation as a teaching axis.** We want students (and instructors)
   to explore *machine design*, not just control code: one vs. two servos per
   station, an optional "smusher" press, and — the interesting one — **a
   different physical layout in each A/B lane**, so a comparison can pit two
   *machines* against each other, not only two control strategies.

**Goals**
- **LG-1** Describe any reasonable line as **data** (an ordered list of modules),
  not code. Adding the capper, a smusher, or a fourth ingredient is a config
  edit, not a contract change.
- **LG-2** Make the controller **layout-aware**: it reads a layout descriptor and
  loops over modules generically, instead of hard-coding graham/choc/marsh. This
  is *more* educational — the interesting logic (hold → actuate → confirm →
  release, dead-reckon between sensors) is the same at every station.
- **LG-3** Let the **end user** pick or edit a layout in the app, from presets or
  a JSON editor, including **per-lane** layouts for the A/B view.
- **LG-4** Preserve every existing invariant: deterministic, unit-testable
  subsystems, the same code runs on the real P1AM by swapping the HAL, and the
  visualizer reads only the exposed contract.

**Non-goals (this iteration)**
- Branching / merging conveyors, or more than one belt. The line stays a single
  ordered belt.
- Arbitrary user-defined *module types* with custom physics. Module **types** are
  a fixed catalog (§L-3); users compose and parameterize instances of them.
- A visual drag-and-drop layout builder. JSON + presets first; a GUI builder is a
  later extension (§L-11).

## L-2. The layout model

A **layout** is an ordered list of **modules** placed along one belt, plus belt-
level parameters. Belt order = physical order = the order a tray encounters them.

```
belt_length_mm, nominal_speed_mm_s, slip (world-only), ...
modules: [
  { id:"g1",  type:"dispenser", pos_mm:300,  ingredient:"graham",     servos:1, confirm:true },
  { id:"c1",  type:"dispenser", pos_mm:600,  ingredient:"chocolate",  servos:1, confirm:true },
  { id:"m1",  type:"dispenser", pos_mm:900,  ingredient:"marshmallow",servos:1, confirm:true },
  { id:"tun", type:"tunnel",    pos_mm:1050, exit_mm:1185, target:"time_at_temp", ... },
  { id:"g2",  type:"dispenser", pos_mm:1320, ingredient:"graham",     servos:1, confirm:true, role:"cap" },
  { id:"sm",  type:"smusher",   pos_mm:1470, dwell_ms:400, servos:1 }   // optional
]
```

- A module has a stable **`id`** (string), a **`type`** from the catalog (§L-3), a
  **`pos_mm`** (its sense/action point on the belt), and type-specific parameters.
- The **recipe** is implied by the ordered dispenser modules: a tray is *complete*
  when every dispenser in belt order has placed its unit (and any required
  smusher/press step has run). "Graham → choc → marsh → toast → top graham" is
  just the module order; nothing in the controller hard-codes it.
- Positions must be strictly increasing; `belt_length_mm` ≥ last module's action
  point. Validation rejects overlaps tighter than a tray width (§L-8).

## L-3. Module type catalog

Each type declares the **inputs** it contributes to the contract and the
**outputs** it consumes (§L-5). Types this iteration:

| Type | Inputs it adds | Outputs it consumes | Params |
|------|----------------|---------------------|--------|
| `dispenser` | `sense` (present), optional `confirm` (drop count) | `gate_open`, `dispense[servo]` (1–2) | `ingredient`, `servos` (1\|2), `confirm` (bool), `dispense_ms`, `role` (`base`\|`cap`\|…) |
| `tunnel` | `entry`, `exit`, `temp_c` | `tunnel_gate_open`, `heater` (bool or duty) | `exit_mm`, `target` (`fixed_dwell`\|`time_at_temp`), `target_value` |
| `smusher` | `sense` (present), optional `confirm` (pressed) | `press` (actuate), optional `gate_open` | `dwell_ms`, `servos`, `force` (visual) |
| `sensor` | `sense` only | — | passive checkpoint (e.g. an extra position fix) |

- **Servos per station (`servos`).** `1` = a single actuator (one flap/auger/gate
  servo). `2` = a dual actuator — modeled as two independent `dispense` channels
  the controller can pulse together (throughput / redundancy) or in sequence. The
  *world* decides what two servos physically do (e.g. two half-doses = one unit,
  or two units); the *contract* just exposes two channels. A 1-servo station
  exposes one channel. This is the concrete reason the contract can't stay `[3]`.
- **Smusher.** A press downstream of a dispenser (typically after the cap) with
  its own actuator and a **dwell**. Effect on the world: marks the tray `pressed`
  (a completion gate; optionally reduces stack height / bonds layers visually). A
  layout may omit it entirely.
- **Capper.** Not a new type — a `dispenser` with `role:"cap"` and, by
  convention, placed after the `tunnel`. `role` is a *hint* for labels/visuals
  and for optional controller policy (e.g. "don't cap until toasted"); the
  mechanics are a normal dispenser.

## L-4. Worked example — the sandwich line (the immediate ask)

The default layout becomes **`sandwich`** (supersedes today's `classic-3`):

```
 g1 graham   c1 choc    m1 marsh    tunnel        g2 graham(cap)   sm smusher
   300 mm      600 mm     900 mm    1050→1185 mm      1320 mm         1470 mm
 ──▮──────────▮──────────▮──────────)))heat(((────────▮───────────────⬓────────▶
```

A finished tray now carries: graham · chocolate · marshmallow · (toasted) ·
graham-cap · (pressed). New controller obligations that fall out for free from a
layout-aware loop:
- Track a tray **through** the tunnel and keep dead-reckoning to reach `g2` — the
  tunnel is no longer the last thing that happens.
- The **contents model** (§5.1) extends to the cap: open-loop counts its cap
  pulse as placed; closed-loop confirms it. A missed cap is a new, visible failure
  mode for the A/B demo (an open sandwich vs. a closed one).

`classic-3` (no cap, no smusher) stays available as a preset for backward
comparison and for the existing tests.

## L-5. Contract impact (the WASM boundary) — the key decision

The §4 contract must stop being fixed-width. Two options; **Option B recommended.**

### Option A — widen the fixed arrays (minimal)
Bump arrays to `MAX_STATIONS` (say 6) and add `press[]`, a second `dispense`
plane for servo 2, and a `station_count`. Cheapest change; the controller still
mostly hard-codes meaning by index. **Rejected as the primary model** because it
doesn't make the machine *described* to the controller — adding the smusher or a
2-servo station still means the controller "knowing" what index 4 is.

### Option B — a layout descriptor + generic per-module channels (recommended)
The controller receives, **once at `init`**, a compact **layout descriptor**, then
reads/writes generic per-module channels each tick. Fixed upper bound
`MAX_MODULES` (e.g. 8) keeps everything statically sized (embedded-friendly, no
loop allocation — preserves §10).

**Init-time descriptor (world → controller, once):**

| Field | Type | Meaning |
|-------|------|---------|
| `module_count` | u8 | number of active modules |
| `module[i].type` | u8 enum | dispenser / tunnel / smusher / sensor |
| `module[i].pos_mm` | f32 | action point |
| `module[i].servos` | u8 | 1–2 (dispensers/smusher) |
| `module[i].flags` | u8 | bit0 has_confirm, bit1 role=cap, … |
| `module[i].aux_mm` | f32 | type-specific (tunnel `exit_mm`) |
| `belt_length_mm`, `nominal_speed` | f32 | belt params |

**Per-tick inputs (world → controller):**

| Field | Type | Meaning |
|-------|------|---------|
| `now_ms`, `run` | u32, bool | unchanged |
| `sense[i]` | bool | presence at module *i* (0 if module has no sensor) |
| `confirm[i]` | u8 | drop/press count at module *i* (0 if no confirm) |
| `proc[i]` | f32 | process value at module *i* (tunnel temp; else 0) |

**Per-tick outputs (controller → world):**

| Field | Type | Meaning |
|-------|------|---------|
| `belt_speed` | f32 | unchanged |
| `gate_open[i]` | bool | gate at module *i* (tunnel uses this as `tunnel_gate_open`) |
| `act[i][s]` | bool | actuator channel *s* (0–1) at module *i*: dispense pulse / press / heater |
| — | | (heater is `act[tunnel][0]`; a 2-servo dispenser uses `act[i][0]`+`act[i][1]`) |

Indices are **belt order**; semantics come from the descriptor, so the controller
never hard-codes "index 3 = marshmallow". Legacy §4 names (`tunnel_entry`,
`heater_on`, …) become documented *views* over this generic array for the
inspector UI. `tunnel_entry`/`tunnel_exit` map to two `sense` points; a tunnel
therefore occupies **two** sensor indices (entry at `pos_mm`, exit at `aux_mm`) —
finalized at implementation.

> This is the single largest change and the reason to spec it before building the
> in-app editor: the controller **API students write against** (§L-7) is shaped by
> this descriptor. Nailing it down first avoids reworking student-facing code.

## L-6. Per-lane layouts (A/B compares machines, not just controllers)

The compare view (§12) already runs two independent worlds on one clock. Extend
the lane config from `{controller}` to `{controller, layout}`:

- **LR-1** Each lane names its own layout **and** controller. Default keeps both
  lanes on the same layout (today's behavior) so a pure controller A/B still works.
- **LR-2** Stimulus stays shared and fair *where layouts agree*: tray injections
  happen at the belt entry of each lane on the same tick; a fault targets a module
  **by `id`** (e.g. "misfire `g1`"), so a fault only applies to lanes whose layout
  has that module. Faults on a module absent in a lane are no-ops (surfaced in the
  log, not silently dropped — see §L-8).
- **LR-3** The tally's definition of "good s'more" is **per-layout** (its own
  complete-recipe check), so lane A (3-station) and lane B (sandwich) are each
  scored against their own spec. The comparison headline becomes explicit, e.g.
  "same controller, +cap+smusher: does the extra actuation cost throughput?"
- **LR-4** Rendering already targets per-lane canvases; each lane draws its own
  module set. No shared-geometry assumption may remain.

## L-7. Impact on the in-app editor & student controllers (ties to G1–G3)

The Monaco editor work (writing JS / Python / C++ controllers in-app) must target
the **layout-aware** API, or student code breaks the moment a layout changes:

- **ER-1** The controller runtime exposes the **descriptor** to student code as a
  read-only `layout` object: `layout.modules[]` with `type`, `pos_mm`, `servos`,
  `ingredient`, `role`, `hasConfirm`. Student logic iterates it (`for (const m of
  layout.dispensers) { … }`) rather than referencing `graham/choc/marsh`.
- **ER-2** Inputs/outputs are addressed by module **index or id**, matching §L-5:
  `io.sense(i)`, `io.setGate(i,open)`, `io.dispense(i,servo)`, `io.confirm(i)`,
  `io.proc(i)`. A tiny helper layer (`byId`, `dispensers`, `tunnel`) keeps
  beginner code readable.
- **ER-3** The **starter controllers** shipped in the editor (open-loop, closed-
  loop) are written against this API and work unchanged across `classic-3`,
  `sandwich`, and a 2-servo layout — demonstrating LG-2 to the student directly.
- **ER-4** Step-debugging (G2/G3) is layout-independent: a line/step callback
  fires regardless of module count, and the "variables" view can show the
  per-module track state the same way at 3 or 6 modules.
- **ER-5 Layout compatibility is validated in every language, matched to when
  that language can catch it.** The C++ path is checked at **compile time** (the
  generated typed contract + `static_assert`s against `layout::` constants — a
  controller that names a module its layout lacks won't build). JS and Python
  controllers have no compile step, so they get an **equivalent runtime check**
  with the same guarantee, surfaced *before the sim advances*, not mid-run:
  - **Declared requirements.** A scripted controller declares what it needs
    (e.g. `requires = { dispensers: ["g1","c1","m1","g2"], tunnel: true,
    smusher: false }`, or simply `layout = "sandwich"`). At load, the runtime
    validates that against the active layout's meta (the `meta()` JSON the
    codegen already emits) and refuses to run on a mismatch, printing a clear
    message ("controller requires module `g2` (graham cap); layout `classic3`
    has no such module") — the runtime analogue of a C++ compile error.
  - **Per-access guard.** The `io` accessors (`io.sense("g2")`, `io.dispense(
    "g2")`, …) throw a descriptive error if the id/index isn't in the layout, so
    even an undeclared access fails loudly instead of silently reading garbage.
  The check is the same idea as the compile-time one — *this controller and this
  layout are a bound pair* — just enforced at the earliest point each language
  allows.

## L-8. Validation & failure surfacing

- **VAL-1** Positions strictly increasing; adjacent action points ≥ one tray width
  apart; `tunnel.exit_mm > tunnel.pos_mm`; `module_count ≤ MAX_MODULES`.
- **VAL-2** Every `dispenser` has an `ingredient`; `servos ∈ {1,2}`; a `cap`-role
  dispenser should be after a `tunnel` (warn, don't hard-fail).
- **VAL-3** Unknown module type, duplicate `id`, or out-of-range position →
  reject with a specific message in the layout editor; never partially apply.
- **VAL-4** A fault targeting an absent module id (§LR-2) logs a visible
  "fault `g2` ignored — not in lane A's layout" rather than disappearing.

## L-9. Functional requirements (delta over §7)

- **LFR-1** The controller reads the layout descriptor at `init` and adapts;
  changing the layout requires re-`init`, not a rebuild.
- **LFR-2** The assembly order is the module order; "complete" = every dispenser
  placed + every required press done, computed from the layout, not constants.
- **LFR-3** A 2-servo dispenser is driven via its two `act` channels; a 1-servo
  station leaves channel 1 unused. The controller must not assume channel count.
- **LFR-4** A tray is tracked continuously across the **whole** belt including
  past the tunnel (so the cap and smusher work) — extends §5/FR-4 past the tunnel.
- **LFR-5** The smusher holds/press-dwells like a station and gates completion if
  present; absent, completion ignores it.
- **LFR-6** Deterministic and statically sized regardless of layout (§10 holds).

## L-10. Config surface (end-user)

- **CFG-1** **Presets** in a dropdown: `classic-3`, `sandwich` (default),
  `sandwich+smusher`, `dual-servo-marsh` (demo of `servos:2`). Selecting one
  re-inits the lane(s).
- **CFG-2** **JSON editor** (reuse the Monaco instance, a `layout.json` buffer)
  with live validation (§L-8) and an "Apply" that re-inits. A malformed layout
  never runs.
- **CFG-3** **Per-lane** pickers in the compare view: lane A / lane B each choose
  preset or custom layout + controller (§L-6).
- **CFG-4** The active layout is reflected in the line view (correct module count,
  labels, positions) and the I/O inspector (rows generated from the descriptor,
  not hard-coded).
- **CFG-5** Layout + controller selection is URL-encodable (shareable demo links)
  — stretch, but keep the config serializable to allow it.

## L-11. Phasing / rollout (recommended)

1. **P1 — descriptor + generic contract (Option B), `classic-3` only.** Refactor
   the C++ controller + the app to the descriptor internally while reproducing
   today's behavior exactly; the integration test (7 cases) must still pass. No
   user-visible change. *This de-risks everything else.*
2. **P2 — `sandwich` layout (the cap).** Add the post-tunnel graham; extend
   tracking past the tunnel (LFR-4) and the completeness/tally check. New test
   cases: cap placed (open+closed loop), missed cap (open-loop ships an open
   sandwich, closed-loop recovers).
3. **P3 — smusher + 2-servo stations.** Add the `smusher` type and `servos:2`
   channel handling; presets `sandwich+smusher`, `dual-servo-marsh`.
4. **P4 — per-lane layouts + config UI.** Layout presets/JSON editor and per-lane
   pickers in compare (§L-6, §L-10). A/B can now compare machines.
5. **P5 — GUI layout builder** (drag modules onto the belt) — future.

Phasing note vs. the editor goal (G1–G3): land **P1's descriptor shape** before
finalizing the student controller API (§L-7), because the API is derived from it.
The Monaco editor shell (G1) and the debugging mechanisms (G2/G3) do not depend on
layout specifics and can proceed in parallel.

## L-12. Open questions / decisions for approval

- **OQ-1 Contract model:** confirm **Option B** (descriptor + generic channels)
  over Option A (wider fixed arrays). *Recommendation: B.*
- **OQ-2 Tunnel sensor indices:** does a tunnel consume two `sense` indices
  (entry/exit) or keep dedicated `tunnel_entry/exit` fields alongside the generic
  arrays? *Recommendation: two generic `sense` indices; legacy names are views.*
- **OQ-3 2-servo semantics:** are two servos "two half-doses = one unit" or "two
  independent units"? Affects the world model + the closed-loop confirm math.
  *Recommendation: make it a per-module param (`servo_mode`), default two-half-
  doses so `servos:2` doesn't silently double a recipe.*
- **OQ-4 Smusher effect:** purely a required step (completion gate) or does it
  change toast/appearance? *Recommendation: completion gate + visual compress
  only; no physics coupling this iteration.*
- **OQ-5 Default layout:** promote `sandwich` to default (this doc assumes yes),
  keeping `classic-3` as a preset — or keep `classic-3` default and ship
  `sandwich` opt-in? *Recommendation: `sandwich` default; it's the fuller lesson.*
- **OQ-6 Real-hardware actuator mapping:** which P1AM module drives a servo /
  press is deferred to the HAL and not asserted here; see
  [../references/README.md](../references/README.md) before making any such claim
  in code (citation policy).
```
