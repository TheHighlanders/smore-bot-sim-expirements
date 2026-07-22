# P1AM Wokwi Teaching Lab — Design & Plan

A browser-simulatable, step-debuggable teaching model of the FACTS Engineering
**ProductivityOpen (P1AM)** industrial controller, its **discrete output
modules**, and the **GPIO shield** — built so students *see* the system operate
(relays turning green) rather than reading SPI bytes.

---

## 1. Goal & requirements

| # | Requirement | How this design meets it |
|---|-------------|--------------------------|
| R1 | Students **see** system operation (relays energize, LEDs light) | Wokwi canvas: 8 green relay-indicator LEDs + a real relay part driven by the base-controller chip; GPIO-shield LEDs/pot/button |
| R2 | **Step-debug** firmware | RP2040 GDB debugging via the Wokwi VS Code extension (`gdbServerPort` in `wokwi.toml`) |
| R3 | **PlatformIO** workflow | `platformio.ini` with a `pico` build env and a `native` test env |
| R4 | Students write **authentic P1AM code** | `P1AM_Sim` exposes the real API (`P1.init()`, `P1.writeDiscrete(...)`) |
| R5 | **Automated integration tests** covering the modules | Two tiers: host Unity tests + Wokwi CI scenarios |
| R6 | Show the module and shield **connect differently** | Discrete module → SPI base controller; GPIO shield → direct MCU pins |

> **Sources & citations.** Every claim below about real hardware or the Wokwi
> toolchain is cited to an offline snapshot under
> [docs/references/](references/) (see [references/README.md](references/README.md)).
> Citation form: `[ref: <file> -> <upstream location>]`, paths relative to
> `docs/references/`.

## 2. The hard constraint (why the design looks like this)

Wokwi does **not** simulate the P1AM's MCU (Microchip SAMD21/SAMD51). Its
supported architectures are ARM/AVR/RISC-V/Xtensa, and the only ARM Cortex-M0+
part listed is the RP2040 — no SAM D part appears anywhere
[ref: wokwi/supported-hardware.md]. The P1AM-100 uses a SAMD21G18
[ref: facts-docs/P1AM-100.md]. Consequences:

- We run on an **RP2040 (Raspberry Pi Pico) stand-in** — supported and
  GDB-debuggable in Wokwi [ref: wokwi/supported-hardware.md,
  wokwi/vscode-debugging.md], and the closest available part to the P1AM's
  Cortex-M0+ MCU.
- The real `P1AM` library is SAMD-specific (SERCOM SPI, fixed pin map) and will
  not compile for RP2040, so we ship **`P1AM_Sim`**, a drop-in fake with the
  same public API [ref: p1am-library.md#discrete-api]. Student code is
  unchanged; only the transport underneath differs.

> Fidelity note: this is a *behavioural* teaching model, not a cycle-accurate
> P1AM. See §6 for exactly where we simplified.

## 3. Architecture

```
        Student sketch (src/main.cpp)  — real P1AM API + Arduino GPIO
                     │
          ┌──────────┴───────────┐
          │                      │
   P1AM_Sim (lib)          Arduino GPIO
   P1.writeDiscrete()      pinMode/digitalWrite/analogRead/analogWrite
          │                      │
   IBaseTransport                │  (GPIO shield = direct pins,
          │                      │   no base controller involved)
   ┌──────┴───────┐              │
   │              │              ▼
SpiTransport   MockTransport   LEDs / button / pot on the Pico
(RP2040 SPI)   (host, tests)
   │              │
   ▼              ▼
 base-controller  base_model.h  ── SAME shared C model on both sides ──┐
 custom chip  ────────────────────────────────────────────────────────┘
 (Wokwi WASM)
   │
   ▼
 S1_01..S1_15 → digital-out LEDs; S2_1..S2_8 → relay module;
 analog/temp slots read from control sliders (see §11)
```

**The two data paths (R6):**

1. **Discrete output module → SPI base controller.**
   `P1.writeDiscrete()` → `SpiTransport` clocks a command frame over real SPI to
   the **base-controller custom chip**, which owns the per-slot output image and
   drives `OUT1..OUT8`. This mirrors real hardware, where the CPU talks to the
   base controller over SPI (CS + ACK), not to the module pins directly
   [ref: facts-docs/P1AM-100.md; pins: p1am-library.md#control-pins]. The
   OUT pins drive the built-in `wokwi-relay-module`, whose green on-board LED
   lights when energized [ref: wokwi/part-relay-module.md].

2. **GPIO shield → direct MCU pins.**
   Plain `digitalWrite`/`analogRead`/`analogWrite` on Pico pins. No SPI, no base
   controller. This is how the real P1AM-GPIO shield works: an 18-position 3.3 V
   terminal block of plain Arduino GPIO, separate from the SPI base
   [ref: facts-docs/P1AM-GPIO.md].

**Single source of truth:** `shared/base_model.h` (plain C) implements
`bm_handle()` — enumeration, write-discrete, read-discrete. It is `#include`d by
**both** the Wokwi chip and the host `MockTransport`. A green host test therefore
exercises the same logic the student watches light up. `shared/module_db.h` is
the shared module catalog (IDs → discrete byte counts), quoted from the real
library's `Module_List.h` [ref: p1am-library.md#module-ids].

## 4. Repository layout

```
p1am-wokwi-lab/
├── platformio.ini              # pico (firmware) + native (tests) envs
├── wokwi.toml                  # firmware/elf paths, gdb port, [[chip]]
├── diagram.json                # Pico + base chip + relay LEDs + GPIO shield parts
├── Makefile                    # host-test / chip / test / clean wrappers
├── src/main.cpp                # student-facing demo sketch
├── lib/P1AM_Sim/               # the fake P1AM library
│   ├── P1AM_Sim.h/.cpp         #   public API (transport-agnostic)
│   ├── SpiTransport.h          #   RP2040 SPI transport (Arduino build)
│   ├── MockTransport.h         #   in-process transport (host tests)
│   └── ShieldLogic.h           #   pure GPIO-shield helpers (testable)
├── shared/                     # #included by BOTH the chip and the tests
│   ├── base_model.h            #   base-controller behaviour + protocol
│   └── module_db.h             #   module catalog (IDs, byte counts)
├── wokwi/
│   ├── chips/p1-base-controller/  # the custom chip (chip.json, src/main.c, Makefile)
│   └── scenarios/*.test.yaml      # Wokwi CI integration scenarios
├── test/                       # PlatformIO Unity tests (pio test -e native)
│   ├── test_discrete_output/
│   └── test_gpio_logic/
├── tools/host_smoke.cpp        # dependency-free smoke test (g++)
└── .github/workflows/ci.yml    # host tests, chip build, fw build, Wokwi scenarios
```

## 5. Protocol (simplified base-controller SPI)

Opcodes match the real library: `MOD_HDR 0x02`, `VERSION_HDR 0x03`,
`READ_DISCRETE_HDR 0x50`, `WRITE_DISCRETE_HDR 0x60`
[ref: p1am-library.md#opcodes -> defines.h:56-77]. Each command is a
**two-phase, ACK-gated** exchange with a **fixed-length** response so the master
always knows how many bytes to clock back:

```
master: wait ACK low
master: CS low ─ send command frame ─ CS high        (phase 1)
chip:   run bm_handle(), drive OUT pins, raise ACK
master: wait ACK high
master: CS low ─ clock N response bytes ─ CS high     (phase 2)
chip:   lower ACK
```

Conventions preserved from the real hardware: **slots 1-based**, **channels
1-based**, discrete data **little-endian**, **channel bit LSB = channel 1**, and
`channel == 0` means "whole-module bitmap" [ref: p1am-library.md#discrete-api;
facts-docs/api_reference.md]. Pin roles (real → Pico stand-in):
`CS A3→GP17`, `SCK D9→GP18`, `MOSI D8→GP19`, `MISO D10→GP16`, `ACK A4→GP20`,
`EN D33→GP21` — real roles from [ref: p1am-library.md#control-pins,
#spi-bus-pins]; Pico pin names from [ref: wokwi/part-pi-pico.md].

## 6. Deliberate simplifications (fidelity ledger)

| Real P1AM | Source | This sim | Why |
|-----------|--------|----------|-----|
| SAMD21/SAMD51 MCU | `facts-docs/P1AM-100.md` | RP2040 stand-in | Wokwi can't simulate SAMD |
| SPI **mode 2** @ 1 MHz, MSB-first | `p1am-library.md#spi-bus` → `P1AM.cpp:27` | SPI **mode 0** | robustness; one-line change to experiment |
| 3-edge ACK pulse (high→low→high) | `p1am-library.md#ack-handshake` → `P1AM.cpp:1296-1331` | simple high/low ACK strobe | easy to reason about while teaching |
| Variable-length responses | `p1am-library.md#discrete-data-model` | fixed-length per opcode (61/4/1) | master needs no pre-knowledge of counts |
| Closed base-controller firmware | — (our model) | open `bm_handle()` model | inspectable + shared with tests |

All `Source` paths are under [docs/references/](references/). Everything a
student's code sees (API, slot/channel semantics, bit layout) is faithful; the
wire timing is simplified.

## 7. Test strategy (R5)

- **Tier 1 — host logic (fast, deterministic, no simulator):**
  `pio test -e native` runs the Unity suites in `test/`; `make host-test` runs a
  dependency-free g++ smoke. These drive `MockTransport` → the same
  `base_model.h` as the chip. This is where per-module coverage lives (add a
  module to `module_db.h`, add a test case).
- **Tier 2 — system integration (Wokwi):** `wokwi/scenarios/*.test.yaml` run
  headless via `wokwi-cli`/`wokwi-ci-action`. `set-control` drives the button;
  `expect-pin` asserts the base chip's `OUT` pins energize; `wait-serial`
  asserts firmware behaviour [ref: wokwi/ci-scenarios.md; action inputs:
  wokwi/actions/wokwi-ci-action.action.yml]. This proves the full
  RP2040 → SPI → chip → pins path end-to-end.

## 8. Verification status (honest)

| Item | Status |
|------|--------|
| Shared model + shim (discrete, analog, temperature) | ✅ unit-tested on host (`make host-test`; discrete/gpio/analog Unity suites) |
| Custom chip C source (4-slot lineup) | ✅ syntax/type-checked with clang (both `-I` and CI include paths) |
| `diagram.json` / `chip.json` | ✅ valid JSON; every `base:` pin ref cross-checked against chip.json; pin names verified against Wokwi docs |
| Makefile / Wokwi toml | ✅ parse-checked |
| WASM chip build | ✅ built with wasi-sdk 33 (`make chip-local`) → valid wasm module exporting `chipInit`, importing the Wokwi API |
| RP2040 firmware build | ⛔ not built here (needs PlatformIO + RP2040 core) |
| End-to-end run in Wokwi (incl. multi-module diagram/scenarios) | ⛔ **not yet run** — see §9 |

## 9. Next steps to bring it up in Wokwi

1. `pio run -e pico` (fetches the RP2040 core) → produces `.pio/build/pico/firmware.uf2`.
2. Build the chip: `make chip` locally (needs clang + wasi-libc) **or** push and
   let CI's `build-chip` job produce `dist/chip.wasm`.
3. Open in VS Code with the Wokwi extension (or `wokwi-cli .`); start the sim,
   then attach the debugger (start sim **first**, then debugger).
4. Run scenarios: `wokwi-cli . --scenario wokwi/scenarios/discrete_output.test.yaml`.
5. **Timing to validate first:** the ACK handshake (phase-1 `done` fires on
   CS-high; `XFER_BUF=96` keeps exactly one `done` per CS cycle). If the master
   ever hangs on `waitAck`, that loop is the place to look.

## 10. Roadmap / extension points

- **More discrete modules:** add a row to `module_db.h` and a lineup entry in the
  chip's `bm_init()` + a Unity case. 8-ch = 1 `doBytes`, 16-ch = 2.
- **Discrete inputs:** model `diBytes`; add a `READ_DISCRETE` source (e.g. DIP
  switches wired to chip input pins) — combo modules (P1-16CDR) already in the DB.
- **Analog output / PWM modules:** extend `bm_handle()` with `0x61`
  (`WRITE_ANALOG`); 4 bytes/channel little-endian, mirroring the read path.
- **Status bytes:** analog modules carry `statusBytes` (over-range, missing 24V);
  add a `READ_STATUS` (`0x40`) path if a lesson needs fault injection.
- **Richer visuals:** give the chip a framebuffer (`display` in `chip.json`) to
  draw an on-chip indicator panel instead of wiring individual LEDs.

## 11. Module coverage (the planned bill of materials)

Every planned part and how it is mocked. Module IDs cited to
`p1am-library.md#module-ids` (→ `Module_List.h`); part kinds to
`facts-docs/<part>.md`.

| Part | Kind | Signs on over SPI? | Module ID | Wokwi mock | Host unit test |
|------|------|--------------------|-----------|-----------|----------------|
| **P1-01DC** | DC power supply | No — powers the base, not enumerated | — | Power only: `3V3`/`VSYS` rails in `diagram.json`. No protocol model (correct — it has none). | n/a |
| **P1AM-200** | CPU | It *is* the controller | — | The RP2040 stand-in running `P1AM_Sim` (Wokwi has no SAMD51). | exercised by every test |
| **P1-15TD1 / TD2** | 15-ch digital out | Yes | `0x14080085` / `0x14080086` | Base chip **slot 1**, pins `S1_01..S1_15` → 15 LEDs | `test_analog_input` (mixed lineup) + `test_discrete_output` bit math |
| **P1-08TRS** | 8-ch relay out | Yes | `0x1404008F` | Base chip **slot 2**, `S2_1..S2_8` → `wokwi-relay-module` | `test_discrete_output` |
| **P1-04AD-2 / ADL-2** | 4-ch analog in | Yes | `0x34605583` / `0x34605590` | Base chip **slot 3**, control sliders `ai_ch1..4` → `READ_ANALOG` | `test_analog_input` |
| **P1-04THM / NTC** | 4-ch temperature | Yes | `0x34608C81` / `0x34608C8E` | Base chip **slot 4**, sliders `temp_ch1..4`; `readTemperature` reinterprets the float | `test_analog_input` |
| **P1AM-GPIO** | GPIO shield | n/a — direct MCU pins | — | Direct Pico pins: button, DO LED, PWM LED, pot (no base controller) | `test_gpio_logic` |
| **P1AM-ETH** *(bonus)* | Ethernet shield | No — uses the CPU's own SPI + CS (WIZnet W5500-class), not the base | — | **Not yet mocked.** See note. | — |

To swap an alternative (e.g. TD2 for TD1, NTC for THM): change the one `bm_init()`
lineup string in the chip and the `MockTransport` lineup in tests — the model
already knows all of them.

**P1AM-ETH note.** The Ethernet shield is a separate SPI peripheral on the CPU
bus (not a P1000 module), and simulating a TCP/IP stack is out of scope for the
"see the system operate" goal. Options if a lesson needs it: (a) a small W5500
stub custom chip that just ACKs SPI so `Ethernet.begin()` doesn't hang, or
(b) defer. Recommend **defer** unless networking is part of the curriculum.

## 12. Unit-test vs. Wokwi mocks — the overlap

They deliberately **share one behavioural model**, so there is no second mock to
keep in sync:

```
                 shared/base_model.h  +  shared/module_db.h
                 (bm_handle, module catalog, I/O images)
                    /                              \
   MockTransport (host)                   base-controller chip (Wokwi WASM)
   -> pio test -e native                  -> real simulated SPI + LEDs/sliders
```

| Layer | Shared? | Where it runs |
|-------|---------|---------------|
| Module catalog (IDs, byte counts) `module_db.h` | **Shared** | both |
| Protocol semantics / I/O images `bm_handle()` | **Shared** | both |
| Transport: direct calls vs SPI + CS/ACK framing | Not shared | unit = `MockTransport`; Wokwi = `SpiTransport` + chip |
| Input source: `setAnalog()` vs control sliders | Not shared (both feed `bm_set_analog`) | unit vs Wokwi |
| Visuals (LEDs, relay, serial), RP2040 Arduino HAL | Wokwi only | Wokwi |

**Practical split:** put per-module *behaviour* coverage in the fast host unit
tests (deterministic, no simulator, no CI-minute quota). Use Wokwi scenarios for
what only the sim can exercise: the SPI wire protocol + ACK timing, the RP2040
firmware path, and the visual/interaction layer. Don't re-assert module data
behaviour in Wokwi — a green unit test already covers the identical `bm_handle`
logic the chip runs.
