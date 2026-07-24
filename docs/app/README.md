# S'mores Line — visualizer (GitHub Pages app)

A single-page, self-contained visualizer for the s'mores-line controller. It runs
the **real** controller: [`controller/src/Controller.cpp`](../../controller/src/Controller.cpp)
compiled to `controller.wasm`, driven tick-by-tick across the JS↔WASM boundary
against a simulated world — the same path the integration test exercises.

- **Open-loop vs closed-loop** — switch controllers, or run them side-by-side in
  Compare view on identical stimulus.
- **Faults** — misfire the graham dispenser, double-drop chocolate, blind the
  marshmallow sensor, jam the chocolate gate. Watch open-loop stay oblivious
  while closed-loop reads `dispense_confirm` and recovers.
- **Decision log** — the controller's own log messages cross the WASM boundary
  via a `host_log` import, so you read the C++ controller's reasoning live.

## Run it locally

The page `fetch()`es `controller.wasm`, which browsers block on `file://`, so it
must be served over http. From the repo root:

```bash
make serve      # builds controller.wasm, assembles the app, serves :8000
```

Then open <http://localhost:8000/>. (`make app` just assembles without serving.)

`controller.wasm` is a build artifact and is **not** committed — `make app`
rebuilds it from source (needs the repo-local wasi-sdk; see
[`tools/get-wasi-sdk.sh`](../../tools/get-wasi-sdk.sh)).

## Deploy

Pushing to `main` triggers [`.github/workflows/pages.yml`](../../.github/workflows/pages.yml),
which compiles the WASM fresh, runs the integration test as a gate, and publishes
this folder to GitHub Pages. Enable it once under **Settings → Pages → Source:
GitHub Actions**.
