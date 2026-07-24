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
├── include/smores/
│   ├── Contract.h     # Inputs/Outputs structs — the controller↔world contract (§4)
│   ├── Hal.h          # abstract hardware interface (per-signal get/set)
│   ├── Subsystems.h   # Conveyor / Station×3 / HeatingTunnel — stateless intent
│   ├── Controller.h   # the one stateful piece (tray tracking + state machine)
│   └── StructHal.h    # HAL backed by the Contract structs (sim/WASM + tests)
├── src/Controller.cpp # control logic (open-loop / closed-loop variants)
├── wasm/main.cpp      # WASM entry: init/tick + shared Inputs/Outputs pointers
└── test/              # host unit tests (g++)
    ├── test_subsystems.cpp   # each subsystem method → the right HAL call
    └── test_controller.cpp   # drive the controller via a fake world, assert outcomes
```

## Design

- **Subsystems** translate a single, stateless intent into a HAL call
  (`station.hold(true)` → close that gate). No line logic lives here, so each
  method is trivially unit-testable against a mock HAL.
- **Controller** owns everything stateful: dead-reckoned tray tracking, the
  per-station hold→dispense→release state machine, and the tunnel-gate toast
  hold. Two registry variants: **open-loop** (assumes each dispense worked) and
  **closed-loop** (reads `dispense_confirm`, retries a miss, flags over/under-fill).
- **HAL swap** is the only thing that changes between targets: `StructHal` for
  the visualizer/tests; a P1AM SPI HAL (reusing the earlier `P1AM_Sim` work) on
  hardware.

## Build & test

```bash
make -C controller host-test   # subsystem + controller logic tests (g++)
make -C controller wasm        # -> build/controller.wasm (needs the repo-local
                               #    wasi-sdk from `make chip-local` at repo root)
```
`make controller-test` from the repo root runs the host tests too.

## Verified

- `test_subsystems` — every intent maps to the expected HAL call.
- `test_controller` — open-loop completes a good s'more; open-loop + a flaky
  graham misfire ships graham-less (oblivious); closed-loop retries and recovers.
- WASM: builds to a valid module exporting `init`, `tick`, `inputs_ptr`,
  `outputs_ptr`, `track_count`, `track_field`.

## Next

Wire `controller.wasm` into [../docs/simulator/mockup.html](../docs/simulator/mockup.html):
JS writes the `Inputs` struct at `inputs_ptr()`, calls `tick()`, reads `Outputs`
at `outputs_ptr()` and the track state — replacing the JS stand-in controller in
each lane with no UI changes. Then iterate the real control logic.
