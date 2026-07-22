# Project guide — P1AM Wokwi Teaching Lab

Educational simulation of the FACTS Engineering ProductivityOpen (P1AM). See
[docs/DESIGN.md](docs/DESIGN.md) for the architecture and [README.md](README.md)
for quick start.

## Citation policy (MANDATORY — applies to every change, human or agent)

This is teaching material. **Every factual claim about how the *real* hardware
or a *third-party tool* behaves MUST be cited to a downloaded, offline source.**
Uncited hardware/tool claims are treated as bugs.

A "claim requiring citation" is any statement of external fact — e.g. "the real
P1AM uses SPI mode 2", a module's ID / channel count / voltage rating, a pin
role, an opcode value, "Wokwi's `spi_init` does X", a part's pin names. Things
that do NOT need a citation: our own design decisions, our sim's simplified
behaviour, and ordinary code logic.

### Rules

1. **Cite inline.** Put a reference tag next to the claim, in the comment or
   prose:
   ```
   [ref: docs/references/<file>#<anchor> -> <upstream file:line or section>]
   ```
   Paths are relative to the repo root. Example (from `shared/base_model.h`):
   `real P1AM uses SPI mode 2 @ 1 MHz [ref: docs/references/p1am-library.md#spi-bus -> P1AM.cpp:27]`.

2. **The source must exist offline** under [`docs/references/`](docs/references/).
   If you introduce a claim from a new source, download it first:
   - Web page → save the raw `.html` (curl) **and** a `.md` extract of the key
     facts, into the right subfolder (`facts-docs/`, `wokwi/`, ...).
   - **PDF → save the `.pdf` AND a `.md`** with the key figures extracted
     (use `pypdf`; note if the text layer won't extract).
   - Source code → prefer a short excerpt in a `.md` reference with `file:line`
     + upstream URL; whole files only if license permits (P1AM is MIT — keep its
     LICENSE alongside any copied file).
   - Every reference file starts with: source URL + `Retrieved: YYYY-MM-DD`.
   Then add it to [docs/references/README.md](docs/references/README.md).

3. **Never invent specifics.** If a datasheet doesn't publish a number, say so
   (see how `datasheets/p1amgpio.md` / `config.md` record "not published")
   rather than filling in a plausible value. Verify against the local copy
   before citing — an earlier draft had wrong module IDs precisely because a
   claim wasn't checked against source.

4. **Separate real vs. sim.** When our sim deviates from real hardware (e.g. SPI
   mode 0 not mode 2; simplified ACK strobe), state both and cite the real one.
   The deviations are tracked in [docs/DESIGN.md](docs/DESIGN.md) §6.

### Reference layout

```
docs/references/
  README.md            # index of every source
  p1am-library.md      # real P1AM protocol/pins/opcodes/module IDs (primary)
  p1am-library/        # MIT snapshots: defines.h, Module_List.h, P1AM.h, LICENSE
  facts-docs/          # facts-engineering.github.io pages (.html + .md)
  datasheets/          # AutomationDirect spec PDFs (.pdf + .md)
  wokwi/               # docs.wokwi.com pages (.html + .md)
```

## Build / test (see README for detail)

- `make host-test` — dependency-free host smoke (g++ only)
- `pio test -e native` — full Unity host tests
- `pio run -e pico` — RP2040 firmware for Wokwi
- `make chip` — build the custom chip to WASM (needs clang + wasi-libc; CI does it)
