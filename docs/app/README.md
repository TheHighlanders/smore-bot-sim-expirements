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
- **✎ Studio — write your own controller** in JavaScript, Python, or C++ against a
  layout-aware `io` API, then Run it on the line and step through it **line by
  line** (🐞 Debug → Line / Tick) with a live state inspector:
  - **JavaScript** runs in-page; stepping instruments the source with a
    `__trace(line)` call and records per-line state.
  - **Python** runs via Pyodide (loaded from CDN on first use); stepping uses
    `sys.settrace`.
  - **C++** is compiled to WebAssembly **entirely in the browser** (a vendored
    clang+lld — run `make wasm-clang` once, ~60 MB, gitignored); stepping uses an
    injected `host_trace(line)` callback reading live state back out of wasm.
  - Each controller declares `requires`; **Check vs layout** validates it against
    the active layout (the runtime analogue of the C++ compile-time contract
    check). Monaco + Pyodide load from a CDN, so the Studio needs network;
    the built-in views are fully offline.
- **Resizable boxes** — every panel edge is draggable: the page width, the
  inspector rail, the Studio's file-tree width, the line view / signal-history /
  editor heights, the log heights, and the Compare-view lane split. Grips appear
  on hover as a short teal
  bar; drag, or focus one and use the arrow keys (`Shift` for fine steps).
  Double-click a grip (or press `Home`) to restore that one dimension; **↺ Reset
  boxes** in the dock restores all of them. Sizes persist per browser under the
  `smores.ui.boxes.v1` localStorage key, and **⧉ Copy sizes** copies them as a
  CSS snippet so a layout you like can be pasted into `index.html` as the new
  default.

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
