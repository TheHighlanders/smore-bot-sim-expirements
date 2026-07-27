# S'mores Line — Controller (C++)

The real controller for the s'mores line, structured exactly as planned in
[../docs/simulator/REQUIREMENTS.md](../docs/simulator/REQUIREMENTS.md): stateless
subsystem classes over a HAL, and one stateful `Controller` that reads inputs,
decides, and writes outputs each tick. Compiles to **WASM** for the browser
visualizer and (later) for the real P1AM over SPI — same code, different HAL.

> This is a faithful **skeleton** ported from the verified JS mockup. The exact
> control logic will evolve; the structure and the §4 contract are the point.

## Layout

```
controller/
├── layouts/           # LAYOUT DEFINITIONS (JSON) — the source of truth for the
│   ├── classic3.json  #   machine topology. A controller is a BOUND PAIR with one
│   └── sandwich.json  #   layout; the layout generates the contract it compiles to.
├── include/smores/
│   ├── generated/     # GENERATED from layouts/<LAYOUT>.json (gitignored):
│   │   ├── Layout.h   #   compile-time geometry/counts (layout::N_DISP, DISP[], …)
│   │   └── Contract.h #   the Inputs/Outputs structs for THIS layout
│   ├── Contract.h     # thin shim -> generated/Contract.h (keeps includes working)
│   ├── Hal.h          # THE HAL: subsystem interfaces (IConveyor/IDispenser/…)
│   ├── Machine.h      # the composition root the controller is handed
│   ├── SimMachine.h   # Sim* HAL impl — backed by the Contract structs (WASM+tests)
│   └── Controller.h   # the one stateful piece (tray tracking + state machine)
├── src/Controller.cpp # control logic (open-loop / closed-loop variants)
├── wasm/main.cpp      # WASM entry: init/tick + shared Inputs/Outputs pointers
└── test/              # host unit tests (g++)
    ├── test_subsystems.cpp   # each subsystem method → the right HAL call
    └── test_controller.cpp   # drive the controller via a fake world, assert outcomes
```

**Layouts.** The machine topology (dispenser positions, the tunnel, servo counts,
a smusher, a post-tunnel graham cap) is described in `layouts/<name>.json` and
compiled by [`tools/layout/codegen.mjs`](../tools/layout/codegen.mjs) into the
typed `generated/Contract.h` + `generated/Layout.h` the controller is written
against — so a controller that names a module its layout lacks won't build. See
[../docs/simulator/LAYOUTS.md](../docs/simulator/LAYOUTS.md). Pick the layout with
`make LAYOUT=sandwich …` (default `classic3`).

## Design

- **The HAL is the subsystem interfaces** ([`Hal.h`](include/smores/Hal.h),
  formalized in [../docs/simulator/HAL.md](../docs/simulator/HAL.md)). The
  controller talks to `IConveyor`/`IDispenser`/`ITunnel` and nothing else — it
  never names a slot, channel, or the contract structs. All module communication
  lives *inside* an implementation, so the same `Controller.cpp` runs in the
  browser (`SimMachine`) and on a real P1AM base (`P1amMachine`, future).
- Each subsystem method is a single stateless intent (`setGate(open)`), so it is
  trivially unit-testable — see [test_subsystems.cpp](test/test_subsystems.cpp).
- **Controller** owns everything stateful: dead-reckoned tray tracking, the
  per-station hold→dispense→release state machine, and the tunnel-gate toast
  hold. Two registry variants: **open-loop** (assumes each dispense worked) and
  **closed-loop** (reads `dispense_confirm`, retries a miss, flags over/under-fill).
- **HAL swap** is the only thing that changes between targets: `SimMachine` for
  the visualizer/tests; a `P1amMachine` whose subsystems hold slot/channel
  bindings from the layout and drive the real base over SPI (reusing the earlier
  [`lib/P1AM_Sim`](../lib/P1AM_Sim/P1AM_Sim.h) façade) on hardware.

## Build & test

```bash
make -C controller host-test    # subsystem + controller logic tests (g++)
make -C controller wasm         # -> build/controller.wasm (needs the repo-local
                                #    wasi-sdk from `make chip-local` at repo root)
make -C controller integration  # build the wasm + drive it through the JS<->WASM
                                # boundary against a simulated world (needs node)
```
`make controller-test` from the repo root runs the host tests too.

## Verified

- `test_subsystems` — every intent maps to the expected HAL call.
- `test_controller` — open-loop completes a good s'more; open-loop + a flaky
  graham misfire ships graham-less (oblivious); closed-loop retries and recovers.
- WASM: builds to a valid module exporting `init`, `tick`, `inputs_ptr`,
  `outputs_ptr`, `track_count`, `track_field`.
- **Integration** (`test/integration.mjs`): the compiled `controller.wasm` is
  instantiated and driven across the WASM↔JS boundary against a JS world — the
  visualizer's exact path. All cases pass: open-loop completes a good s'more;
  open-loop + flaky graham ships graham-less while believing it placed it;
  closed-loop retries and recovers.

## Next

Wire `controller.wasm` into [../docs/simulator/mockup.html](../docs/simulator/mockup.html):
JS writes the `Inputs` struct at `inputs_ptr()`, calls `tick()`, reads `Outputs`
at `outputs_ptr()` and the track state — replacing the JS stand-in controller in
each lane with no UI changes. Then iterate the real control logic.
