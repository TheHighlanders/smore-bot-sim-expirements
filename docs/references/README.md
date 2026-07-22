# Offline reference library

Downloaded, offline snapshots of every external source this project cites, so
factual claims about the real hardware and the Wokwi toolchain are checkable
without a network connection. **All snapshots retrieved 2026-07-22.**

See the [citation policy](../../CLAUDE.md) for how to cite these and how to add
new sources. Inline citations in the code look like:
`[ref: docs/references/<file>#<anchor> -> <upstream file:line>]`.

## What's here

| Path | Covers | Upstream |
|------|--------|----------|
| [p1am-library.md](p1am-library.md) | **Primary** ref for real P1AM protocol: SPI mode/clock, control pins, opcodes, the 3-edge ACK handshake, discrete API, module IDs & data model | github.com/facts-engineering/P1AM v1.0.10 (MIT) |
| [p1am-library/](p1am-library/) | MIT source snapshots quoted above: `defines.h`, `Module_List.h`, `P1AM.h`, `README.md`, `LICENSE` | same |
| [facts-docs/](facts-docs/) | HTML + `.md` of the FACTS doc pages: `api_reference`, `P1AM-100`, `P1AM-GPIO`, `config`, and the discrete module pages (`P1-08TRS`, `P1-16TR`, `P1-08TD1/2`) | facts-engineering.github.io |
| [datasheets/](datasheets/) | AutomationDirect spec PDFs + `.md` extracts: `p1amgpio`, `P1-08TRS`, `P1-08TD1` | cdn.automationdirect.com |
| [wokwi/](wokwi/) | HTML + `.md` of docs.wokwi.com: supported-hardware, Chips API (spi/gpio/chip-json/framebuffer), the parts we use, VS Code debugging/config, and CI/scenarios | docs.wokwi.com |

Per-folder indexes: [facts-docs/_index.md](facts-docs/_index.md) ·
[datasheets/_index.md](datasheets/_index.md) · [wokwi/_index.md](wokwi/_index.md).

## Known gaps (recorded, not invented)

- `facts-docs/config.html` is a JS-rendered tool; its client-side module list
  didn't extract. The "discrete modules need no configuration" fact is instead
  taken verbatim from each discrete module's own page (see `facts-docs/config.md`).
- The `datasheets/p1amgpio.pdf` spec sheet does **not** publish ESD / PTC-fuse /
  per-pin numbers; those figures come from the `facts-docs/P1AM-GPIO` HTML page
  instead. Where a number isn't published anywhere, the `.md` says so.

## Fidelity note

These sources describe the **real** hardware/tools. Where our simulation
deliberately simplifies (e.g. SPI mode 0 vs. the real mode 2, a simple ACK
strobe vs. the real 3-edge pulse), that is called out at the citation and
tracked in [docs/DESIGN.md](../DESIGN.md) §6.
