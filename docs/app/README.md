# S'mores Line — visualizer (GitHub Pages app)

A single-page, self-contained visualizer for the s'mores-line controller. It runs
the **real** controller: [`controller/src/Controller.cpp`](../../controller/src/Controller.cpp)
compiled to `controller.wasm`, driven tick-by-tick across the JS↔WASM boundary
against a simulated world — the same path the integration test exercises.

- **Layouts** — pick the machine topology from the **Layout** dropdown. Each
  layout is a `layout.json` that *generated* its own `controller.wasm` (a bound
  pair); the visualizer rebuilds the line, the I/O contract rows, and the fault
  buttons from the selected layout's meta. Ships with `sandwich` (default —
  graham → choc → marsh → **toast** → graham **cap** after the tunnel) and
  `classic3` (the original 3-station line).
- **Open-loop vs closed-loop** — switch controllers, or run them side-by-side in
  Compare view on identical stimulus.
- **Faults (per module)** — cycle any dispenser ok → FAIL (misfire) → DOUBLE,
  blind the marshmallow sensor, or jam the chocolate gate. Watch open-loop stay
  oblivious while closed-loop reads `dispense_confirm` and recovers.
- **Decision log** — the controller's own log messages cross the WASM boundary
  via a `host_log` import, so you read the C++ controller's reasoning live.

## Run it locally

The page `fetch()`es each layout's `wasm/<name>.wasm` + `layouts/<name>.layout.json`,
which browsers block on `file://`, so it must be served over http. From the repo
root:

```bash
make serve      # builds every layout's wasm + meta, assembles the app, serves :8000
```

Then open <http://localhost:8000/>. (`make app` just assembles without serving.)

The compiled wasm + generated layout meta are build artifacts and are **not**
committed — `make app` rebuilds them from source (needs the repo-local wasi-sdk;
see [`tools/get-wasi-sdk.sh`](../../tools/get-wasi-sdk.sh)). The set of layouts is
`APP_LAYOUTS` in the root [`Makefile`](../../Makefile); add a layout by dropping a
`controller/layouts/<name>.json` and listing it there + in `LAYOUT_CHOICES` in
`index.html`.

## Deploy

Pushing to `main` triggers [`.github/workflows/pages.yml`](../../.github/workflows/pages.yml),
which compiles the WASM fresh, runs the integration test as a gate, and publishes
this folder to GitHub Pages. Enable it once under **Settings → Pages → Source:
GitHub Actions**.
