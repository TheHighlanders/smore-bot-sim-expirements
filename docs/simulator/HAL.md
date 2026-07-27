# S'mores Line — HAL & Hardware Binding: Requirements

Status: **draft for approval** (2026-07-27). Extends
[REQUIREMENTS.md](REQUIREMENTS.md) (§3 architecture, §4 contract) and
[LAYOUTS.md](LAYOUTS.md) (topology as data). Section IDs here are `H-n`.

This doc formalizes two things that are currently informal:

1. **Where the hardware abstraction boundary sits.** It moves *up*, to the
   **subsystem level**. The controller a student writes must be the code that
   runs on the real P1AM, unchanged; all module communication (SPI, slots,
   channels) is abstracted away *inside* the subsystem implementations.
2. **How a layout binds to real hardware.** The layout gains an I/O binding
   (which module, slot and channel each subsystem signal lives on) so the
   hardware implementation can be *generated* and *validated*, not hand-written.

It also answers the monitoring question (§H-8): what has to exist in controller
code for the UI to inspect values in memory.

---

## H-1. Why the seam moves to the subsystem level

Today there are two abstraction layers, and the lower one is at the wrong
altitude:

```
  NOW:  Controller ──> Subsystems ──> Hal (per SIGNAL) ──> StructHal
                                       ^^^^^^^^^^^^^^^^
                     setGate(stationIndex, open), trayPresent(stationIndex), …
```

A per-signal HAL flattens the machine into a global signal index. That is a poor
fit for real hardware, where a signal is identified by **(slot, channel)** on a
specific module — so the hardware layer would have to re-derive slots from an
index, and the natural unit of wiring (one station's gate + actuator + sensors,
often sharing a module) is lost.

```
  PROPOSED:
    Controller.cpp                        ← STUDENT CODE. Identical on sim + hardware.
        │  talks ONLY to subsystem interfaces
        ▼
    HAL = IConveyor · IDispenser · ITunnel · ISmusher · IClock     ← the swap point
        ├── Sim*   : reads/writes the generated Inputs/Outputs structs   (browser/WASM)
        └── P1am*  : owns its (slot, channel) binding; issues P1.* calls (real base)
```

**H-1.1** The HAL *is* the set of subsystem interfaces. There is no per-signal
HAL; `Hal.h` and `StructHal.h` are removed.
**H-1.2** A subsystem implementation is the **only** place module communication
appears. `Sim*` implementations touch the contract structs; `P1am*`
implementations touch the P1AM API (§H-5).
**H-1.3** The controller must not name a slot, channel, module, opcode, or the
`Inputs`/`Outputs` structs. Those are implementation details of the HAL.

## H-2. The HAL interfaces (normative)

One interface per subsystem **type**. Methods are stateless intents, so each is
trivially unit-testable against a mock (as today's subsystem tests already do).

```cpp
namespace smores {

struct IClock {                       // time + operator run/E-stop
    virtual uint32_t nowMs()  = 0;
    virtual bool     running() = 0;
    virtual ~IClock() {}
};

struct IConveyor {
    virtual void setSpeed(float mm_s) = 0;   // 0 = stopped
    virtual ~IConveyor() {}
};

struct IDispenser {
    virtual bool    trayPresent()                  = 0;   // presence sensor
    virtual uint8_t confirmedDrops()               = 0;   // 0 if no confirm sensor fitted
    virtual void    setGate(bool open)             = 0;
    virtual void    runActuator(int servo, bool on) = 0;   // servo in [0, servos)
    virtual int     servos() const                 = 0;    // 1 or 2 (from the layout)
    virtual ~IDispenser() {}
};

struct ITunnel {
    virtual bool  atEntry()          = 0;
    virtual bool  atExit()           = 0;
    virtual float temperatureC()     = 0;
    virtual void  setGate(bool open) = 0;    // hold a tray to dwell/toast
    virtual void  setHeater(bool on) = 0;
    virtual ~ITunnel() {}
};

struct ISmusher {                     // optional press (LAYOUTS.md §L-3)
    virtual bool trayPresent()      = 0;
    virtual bool pressConfirmed()   = 0;
    virtual void setGate(bool open) = 0;
    virtual void press(bool on)     = 0;
    virtual ~ISmusher() {}
};

} // namespace smores
```

**H-2.1** Every method is a single intent with no line logic.
**H-2.2** A capability a given unit does not have degrades safely, it does not
crash: `confirmedDrops()` returns 0 where no confirm sensor is fitted, and the
layout tells the controller whether to trust it (`has_confirm`).
**H-2.3** Interfaces are versioned with the layout schema; adding a method is a
breaking change requiring a schema bump (§H-11).

> **Design note — virtual dispatch.** Virtuals cost a vtable and an indirect call
> per access. On a SAMD-class MCU at a 20 ms control period this is irrelevant,
> and the clarity/testability win is large. If profiling ever says otherwise, the
> same interfaces can be made compile-time (templates/CRTP) without touching
> controller source.

## H-3. The `Machine` — a generated composition root

The controller iterates *subsystems*, never indices. The layout generates the
composition:

```cpp
struct Machine {
    IClock&     clock;
    IConveyor&  belt;
    IDispenser* disp[layout::N_DISP];      // in belt order
    ITunnel*    tunnel;                    // nullptr if the layout has none
    ISmusher*   smusher;                   // nullptr if absent
};
```

**H-3.1** `Machine` is generated from the layout (count, order, presence), so
`layout::N_DISP` and the array stay in lockstep with the JSON.
**H-3.2** Two factories are generated: `makeSimMachine()` and
`makeP1amMachine()`. Only the factory differs between targets.
**H-3.3** Ordering is belt order, so `disp[i]` corresponds to `layout::DISP[i]`
(position, ingredient, stage, `after_tunnel`, `is_cap`).

## H-4. What the student writes

The student's file is portable by construction, and top-level entry is
Arduino-shaped so it reads like firmware rather than a WASM export:

```cpp
#include "smores/Machine.h"     // generated: layout:: constants + Machine
#include "smores/Controller.h"

static Machine&   machine = defaultMachine();      // Sim* or P1am*, chosen at build
static Controller controller(machine);

void setup() { machine.begin(); }                  // real build also: while(!P1.init()){}
void loop()  { controller.update(); }              // browser harness calls update() directly
```

**H-4.1** `Controller.cpp` compiles unmodified against both machine
implementations. This is a testable claim, and CI must test it (§H-10).
**H-4.2** The controller reads geometry/recipe from generated `layout::`
constants (already true today) and I/O through `Machine` (new).
**H-4.3** The browser harness calls `update()` in place of `loop()`; the
student's file is identical either way.

## H-5. The two implementations

### H-5.1 `Sim*` (browser / WASM / host tests)
Backed by the generated `Inputs`/`Outputs` structs — i.e. exactly what
`StructHal` does today, redistributed into per-subsystem classes. This is the
implementation the visualizer drives across the WASM boundary.

### H-5.2 `P1am*` (real base controller, and Wokwi)
Holds its own binding from the layout (§H-6) and issues P1AM API calls. Slot and
channel numbering is **1-based**, and channel `0` means "the whole module as a
bitmap, LSB = channel 1"
[ref: docs/references/p1am-library.md#discrete-api -> P1AM.h:45-51; bitmap detail:
docs/references/facts-docs/api_reference.md] — the convention our existing
faithful façade already documents (`lib/P1AM_Sim/P1AM_Sim.h:17-20`).

```cpp
class P1amDispenser : public IDispenser {
    // binding injected from the generated layout (slot/channel per signal)
    void setGate(bool open) override { P1.writeDiscrete(open ? HIGH : LOW, gate_.slot, gate_.channel); }
    bool trayPresent()      override { return P1.readDiscrete(sense_.slot, sense_.channel) != 0; }
    // …
};
```

**H-5.3** On hardware this links the real FACTS `P1AM` library; in Wokwi it links
[`lib/P1AM_Sim`](../../lib/P1AM_Sim/P1AM_Sim.h), the drop-in façade already built
and cited in this repo. Both present the same call surface, so `P1am*` source is
shared.
**H-5.4** Modules requiring configuration must be configured at `begin()` before
any read. Our offline module table records non-zero config sizes for the analog
and temperature parts — `P1-04AD-2` (2), `P1-04NTC` (8), `P1-04THM` (20)
[ref: shared/module_db.h -> docs/references/p1am-library/Module_List.h], so a
layout that binds a temperature input **must** also carry that module's config
(§H-6.3).

## H-6. Layout v2 — hardware binding

The layout currently describes physics only, so no hardware implementation can be
generated from it. v2 adds an inventory and a per-signal binding.

```json
{
  "schema": 2,
  "base": [
    { "slot": 1, "part": "P1-15TD1" },
    { "slot": 2, "part": "P1-08ND3" },
    { "slot": 3, "part": "P1-04THM", "config": { "…": "see H-6.3" } }
  ],
  "modules": [
    { "id": "g1", "type": "dispenser", "pos_mm": 300, "ingredient": "graham",
      "servos": 1, "confirm": true,
      "io": {
        "sense":    { "slot": 2, "channel": 1 },
        "gate":     { "slot": 1, "channel": 1 },
        "actuator": [ { "slot": 1, "channel": 2 } ],
        "confirm":  { "slot": 2, "channel": 2 }
      } }
  ]
}
```

**H-6.1** `base[]` is the physical module inventory by slot; `part` must exist in
the module catalog (`shared/module_db.h`).
**H-6.2** Each subsystem's `io` maps its **logical signals** (named by the HAL
interface: `sense`, `gate`, `actuator[]`, `confirm`, `entry`, `exit`, `temp`,
`heater`, `press`) to `{slot, channel}`. `actuator` is an array, one entry per
servo, so `servos: 2` requires two bindings.
**H-6.3** A module whose catalog entry has `configBytes > 0` must supply
`config`. The exact field set is per part and must come from the offline
references, not be invented (§H-12 OQ-3).
**H-6.4** `io` is **optional** for a sim-only layout. Omitted ⇒ the layout builds
`Sim*` only, and generating `P1am*` is refused with a clear message. This keeps
today's layouts working (§H-11 P1).
**H-6.5** The binding is part of the bound pair: changing it re-generates and
requires a rebuild, exactly like changing topology.

## H-7. Binding validation (what codegen must reject)

Because the module catalog records channel counts and I/O direction per part, the
binding is checkable at generate/compile time — extending LAYOUTS.md's bound-pair
rule from *logical* modules to *physical wiring*. Codegen rejects, with the
offending id and a specific message:

- **VAL-H1** unknown `part`, or duplicate/missing `slot`
- **VAL-H2** `channel` outside `1..channels` for that part
- **VAL-H3** direction mismatch — an output signal (`gate`, `actuator`,
  `heater`, `press`) bound to a module with `doBytes == 0`, or an input signal
  (`sense`, `confirm`, `entry`, `exit`) bound to one with `diBytes == 0`
- **VAL-H4** `temp` bound to a module with `aiBytes == 0`
- **VAL-H5** two signals bound to the same `{slot, channel}` (collision)
- **VAL-H6** a subsystem missing a signal its interface requires (e.g. a
  dispenser with `confirm: true` and no `confirm` binding), or `actuator` count
  != `servos`
- **VAL-H7** `configBytes > 0` for a bound part with no `config` supplied

**H-7.1** A mis-wired layout must fail the build, not fail at runtime.
**H-7.2** The same validator runs in the browser (§H-9) so the UI reports the
identical error before Apply.

## H-8. Monitoring values in memory

**Question:** does the controller need extra code for the UI to watch its state?
**Answer:** for its *declared* state, no — if we generate the memory map.
For locals/derived values, yes, and that's healthy.

### H-8.1 Declared state block (no code in the controller)
Extend the generated contract from `{Inputs, Outputs}` to
`{Inputs, Outputs, State}`. Codegen already computes exact wasm32 struct offsets,
and the visualizer already decodes I/O generically from that map — point the same
machinery at a generated state struct plus a `state_ptr()` export and the UI can
display **any field with no per-field accessor**.

This **replaces `track_field()`**, the hand-written switch that is currently the
only window into controller state — and which silently capped at 3 dispensers
until the post-tunnel cap exposed it. Generated maps do not drift from the struct.

### H-8.2 `WATCH` for anything not in that struct
```cpp
WATCH("drift_mm", corr);     // -> host_watch(name, value); same import style as host_log
```
**H-8.2.1** `WATCH` is a no-op on hardware builds by default, and can instead
stream over serial or P1AM-ETH — so it is *not* sim-only scaffolding. It mirrors
established practice (PLC tags exposed to an HMI; AdvantageKit-style logged
values), which is worth teaching explicitly.
**H-8.2.2** Watches are timestamped per tick, so they can feed the existing
signal-history timeline, not just a text panel.

### H-8.3 Rejected: DWARF-based inspection
Reading arbitrary locals with *zero* code changes means shipping a DWARF parser,
mapping wasm frames to source, and trusting debug info from the vendored 2019
in-browser clang. Weeks of work and a fragile dependency, for a capability the
declared state block plus `WATCH` already covers pedagogically. Revisit only if
the toolchain is upgraded (see the note in `docs/app/README.md`).

## H-9. Editing model in the UI

Decision (2026-07-27): **layout is edited as validated JSON**; the project is a
**fully editable file tree**, so the sim setup and the real setup stay
harmonized and inconsistencies surface early. Near-term work focuses on the
controller authoring workflow.

- **UI-1** The Studio becomes a small **project tree**, not a single buffer:
  `layout.json`, `Controller.cpp` (default), the subsystem interfaces, and the
  `Sim*` / `P1am*` implementations — each its own Monaco model.
- **UI-2** `layout.json` gets **live validation** in the editor: the schema and
  wiring checks of §H-7, reported inline, with an **Apply** that regenerates the
  contract and re-inits the lane. A layout that fails validation never runs.
- **UI-3** The validator is **one implementation** shared by codegen (Node) and
  the browser — `tools/layout/codegen.mjs` already exports pure functions for
  exactly this reason.
- **UI-4** Layout JSON is **import/export**able as a file so a student's
  controller and its layout can be committed together (LAYOUTS.md CFG-2).
- **UI-5** Editing `P1am*` sources is permitted; the *value* is that a mismatch
  between the sim wiring and the real wiring becomes visible in the same place.
  Compiling/unit-testing the module-level code in-browser is a **future**
  extension (§H-11 P5), not required now.
- **UI-6** Read-only generated files (`Layout.h`, `Contract.h`) are shown as
  such, with a pointer to the layout that generates them.

## H-10. Portability guarantee & CI

The central claim is "the controller you wrote runs on the real base". Make it
mechanical, not aspirational:

- **CI-H1** Compile the **same** `Controller.cpp` twice: against
  `makeSimMachine()` (host + wasm) and against `makeP1amMachine()` (Wokwi/RP2040
  build via `lib/P1AM_Sim`). A controller that only builds for the sim fails CI.
- **CI-H2** Run the existing subsystem unit tests against mocks of the new
  interfaces, and the controller logic tests unchanged.
- **CI-H3** Run the WASM integration test per layout, as today (`classic3`,
  `sandwich`).
- **CI-H4** Add a binding-validation test suite: a table of deliberately bad
  layouts, each asserting the specific §H-7 error.

## H-11. Phasing

1. **P1 — HAL refactor, behavior-preserving.** Introduce the interfaces +
   generated `Machine` + `Sim*`; delete `Hal.h`/`StructHal.h`; port the
   subsystem/controller tests. Layouts unchanged (`io` optional, §H-6.4). All
   existing tests must stay green.
2. **P2 — telemetry.** Generated `State` block + `state_ptr()`; delete
   `track_field()`; add `WATCH`. Wire the debugger/inspector to the generated map.
3. **P3 — layout v2 binding + validation.** `base[]`/`io` schema, §H-7
   validators, shared with the browser; generate `P1am*` wiring.
4. **P4 — Studio project tree.** Multi-file editing, `layout.json` tab with live
   validation + Apply (§H-9).
5. **P5 — module-level tests in-browser (future).** Compile and run unit tests
   over the subsystem/HAL code in the Studio.

**Sequencing note:** P1 and P2 are the controller-authoring workflow the user
prioritized. P3 is what makes the real-hardware story real; it is independent of
P4 and can land first.

## H-12. Open questions

- **OQ-1 Interface granularity.** Is `IDispenser` right, or should gate and
  actuator be separate interfaces (`IGate`, `IActuator`) that a dispenser
  composes? Composition is more flexible for odd machines; one interface per
  station is simpler to teach. *Recommendation: keep `IDispenser`; revisit if a
  layout needs a gate without a dispenser.*
- **OQ-2 Who owns dwell/timing constants** (`dispense_ms`, `toast_ms`)? Today
  they are controller `Config`. They are arguably machine properties belonging in
  the layout. *Recommendation: move to the layout, since two different machines
  should be able to differ; keep a controller override.*
- **OQ-3 Module config field sets.** The exact configuration payload for
  `P1-04THM` / `P1-04NTC` / `P1-04AD-2` must be read from the offline references
  before §H-6.3 is implemented; if a field is not published offline, record the
  gap rather than invent it (CLAUDE.md rule 3).
- **OQ-4 `WATCH` on hardware.** No-op, serial, or P1AM-ETH by default?
  *Recommendation: no-op by default, opt-in serial, to avoid surprising timing
  cost in a control loop.*
- **OQ-5 Schema migration.** Auto-upgrade v1 layouts in the UI, or require an
  explicit `"schema": 2`? *Recommendation: treat missing `schema` as 1 and accept
  it (sim-only), so nothing in the repo breaks.*
