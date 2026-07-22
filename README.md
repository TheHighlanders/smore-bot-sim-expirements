# P1AM Wokwi Teaching Lab

A visual, step-debuggable simulation of the FACTS Engineering **ProductivityOpen
(P1AM)** controller — a **discrete output relay module** (over the SPI base
controller) and the **GPIO shield** (direct pins) — for teaching. Students write
authentic P1AM code, press a button, and **watch relays turn green**.

> **Why a stand-in MCU?** Wokwi can't simulate the P1AM's SAMD21/SAMD51
> ([ref](docs/references/wokwi/supported-hardware.md)), so this runs on an
> **RP2040 (Pi Pico)** stand-in. Student code is unchanged: it uses the real
> P1AM API via a drop-in shim (`P1AM_Sim`). Full rationale and the fidelity
> ledger are in [docs/DESIGN.md](docs/DESIGN.md).

> **Citations.** This is teaching material: every claim about real hardware or
> Wokwi is cited to an offline snapshot in
> [docs/references/](docs/references/README.md). New claims must follow the
> [citation policy](CLAUDE.md).

## What's in the box

- **`P1AM_Sim`** — drop-in fake of the P1AM library (`P1.init()`,
  `P1.writeDiscrete()`, `P1.readDiscrete()`, `P1.readAnalog()`, `P1.readTemperature()`).
- **`p1-base-controller`** — a Wokwi custom chip that speaks the base-controller
  SPI protocol and models one of each planned module: 15-ch digital out
  (P1-15TD1), 8-ch relay (P1-08TRS), 4-ch analog in (P1-04AD-2), 4-ch
  thermocouple (P1-04THM). See the [module coverage table](docs/DESIGN.md).
- **GPIO shield demo** — button, digital-out LED, PWM LED, potentiometer on
  direct Pico pins.
- **Two test tiers** — host Unity tests + headless Wokwi scenarios, over one
  shared model (see [unit-vs-Wokwi overlap](docs/DESIGN.md)).

## Quick start

```bash
# 1. Host logic tests — no board, no simulator, just a C++ compiler
make host-test           # dependency-free smoke
pio test -e native       # full Unity suite (needs PlatformIO)

# 2. Build the RP2040 firmware for Wokwi
pio run -e pico          # -> .pio/build/pico/firmware.uf2

# 3. Build the custom chip to WASM (needs clang + wasi-libc; CI does this for you)
make chip                # -> wokwi/chips/p1-base-controller/dist/chip.wasm
```

Then open the folder in VS Code with the **Wokwi extension** and start the
simulator (`wokwi.toml` + `diagram.json` are at the repo root), or run headless:

```bash
wokwi-cli . --scenario wokwi/scenarios/discrete_output.test.yaml
```

## Try it (in the simulator)

- Turn the **potentiometer** → the blue **PWM LED** dims/brightens.
- Press the **button** → the yellow **DO LED** mirrors it (direct GPIO), *and*
  the next **green relay indicator** energizes (SPI → base controller → module).
  This is the whole lesson: two outputs, two very different signal paths.

## Running in Wokwi — step by step

Two build outputs are needed before Wokwi can run: the **RP2040 firmware**
(PlatformIO) and the **base-controller chip compiled to WASM**. The chip WASM is
the only step that needs a special toolchain — the dev container (below) or CI
provide it.

**Prerequisites:** VS Code with the **PlatformIO IDE** and **Wokwi for VS Code**
extensions (both recommended in `.vscode/extensions.json`). The Wokwi extension
needs a free license once: command palette → `Wokwi: Request a new License`.

1. **Firmware:** `pio run -e pico` → `.pio/build/pico/firmware.uf2` + `.elf`
   (the paths `wokwi.toml` expects).
2. **Custom chip → WASM.** Pick one:
   - **Host build with wasi-sdk (recommended, esp. on a corporate network):**
     download a [wasi-sdk](https://github.com/WebAssembly/wasi-sdk/releases)
     release for your host (e.g. `wasi-sdk-24.0-arm64-macos.tar.gz`), unpack it,
     then:
     ```bash
     make chip CLANG=/path/to/wasi-sdk/bin/clang \
               WASI_ROOT=/path/to/wasi-sdk/share/wasi-sysroot
     ```
     The download runs on your host, which already trusts your corporate CA.
   - **CI artifact (no local toolchain at all):** push to GitHub; the
     `build-chip` job runs without any Wokwi token and uploads a `chip`
     artifact — unzip `chip.wasm` + `chip.json` into
     `wokwi/chips/p1-base-controller/dist/`.
   - **Dev container:** “Dev Containers: Reopen in Container”, then `make chip`.
     ⚠️ If your network intercepts TLS (e.g. Zscaler), the in-container wasi-sdk
     download fails with `curl (60) unable to get local issuer certificate`.
     Fix (macOS): run `.devcontainer/import-host-certs.sh` to export your host
     root CAs into [.devcontainer/certs/](.devcontainer/certs/), then **Dev
     Containers: Rebuild Container**. (Those `*.crt` are gitignored.) Or just use
     the host/CI options above.
3. **Run:** open [diagram.json](diagram.json) and press ▶ (or `Wokwi: Start
   Simulator`). It reads `wokwi.toml`, loads the firmware + chip, and starts.

### Step debugging

`wokwi.toml` sets `gdbServerPort = 3333` and [.vscode/launch.json](.vscode/launch.json)
has a matching `Wokwi: Debug RP2040 (GDB)` config. **Start the simulator first,
then press F5** — breakpoint in `src/main.cpp`, step through `P1.writeDiscrete()`
into the transport. If GDB can't launch, set `miDebuggerPath` to your
`arm-none-eabi-gdb` (PlatformIO installs one under `~/.platformio/packages/`).

## Layout

See [docs/DESIGN.md §4](docs/DESIGN.md) for the full tree and the architecture.
Short version: student code in `src/`, the fake library in `lib/P1AM_Sim/`, the
shared base-controller model in `shared/` (used by *both* the chip and the
tests), the chip in `wokwi/chips/`, tests in `test/`, scenarios in
`wokwi/scenarios/`.

## Status

Host logic is compiled and unit-tested; the chip C is type-checked; JSON/configs
are validated. The end-to-end Wokwi run has **not** been executed yet — the chip
WASM and RP2040 firmware still need to be built (locally or in CI). See
[docs/DESIGN.md §8–9](docs/DESIGN.md) for the verification ledger and bring-up
steps.
