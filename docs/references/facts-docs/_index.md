# FACTS Engineering — offline reference snapshots

Offline reference copies of FACTS Engineering P1AM documentation pages, retrieved
**2026-07-22** for an educational project so that claims in code can be cited to a
local file. Each page has a faithful raw `.html` (from `curl`) and a distilled
`.md` of the key technical facts. See `_FETCH_NOTES.md` for fetch caveats.

| Files | Source URL |
|-------|-----------|
| `api_reference.html` / `api_reference.md` | https://facts-engineering.github.io/api_reference.html |
| `P1AM-100.html` / `P1AM-100.md` | https://facts-engineering.github.io/modules/P1AM-100/P1AM-100.html |
| `P1AM-GPIO.html` / `P1AM-GPIO.md` | https://facts-engineering.github.io/modules/P1AM-GPIO/P1AM-GPIO.html |
| `config.html` / `config.md` | https://facts-engineering.github.io/config.html |
| `P1-08TRS.html` / `P1-08TRS.md` | https://facts-engineering.github.io/modules/P1-08TRS/P1-08TRS.html |
| `P1-16TR.html` / `P1-16TR.md` | https://facts-engineering.github.io/modules/P1-16TR/P1-16TR.html |
| `P1-08TD1.html` / `P1-08TD1.md` | https://facts-engineering.github.io/modules/P1-08TD1/P1-08TD1.html |
| `P1-08TD2.html` / `P1-08TD2.md` | https://facts-engineering.github.io/modules/P1-08TD2/P1-08TD2.html |

## Support files

- `_index.md` — this file.
- `_FETCH_NOTES.md` — fetch caveats (notably: `config.html` is JS-rendered).

## Quick fact map (where to cite what)

- **API signatures / argument order, slot & channel 1-based, channel 0 = bitmapped
  (LSB = channel 1), `init()` returns module count** → `api_reference.md`
- **SAMD21G18 MCU, SPI-to-Base-Controller, 5 SPI pins (D8 MOSI, D9 CLK, D10 MISO,
  A3 CS, A4 ACK), 24V requirement** → `P1AM-100.md`
- **Plain Arduino GPIO (not the SPI base), 18-position terminal block, 3.3V, full
  pin/terminal map, protection except DAC0, PTC fuse (trip 420 mA / hold 200 mA)**
  → `P1AM-GPIO.md`
- **Which modules need configuration; discrete modules do not** → `config.md`
  (principle cross-cited to the discrete-module files below)
- **P1-08TRS: 8-pt relay, 6× Form A + 2× Form C, no status data / no config** →
  `P1-08TRS.md`
- **P1-16TR: 16-pt relay, Form A, no status data / no config** → `P1-16TR.md`
- **P1-08TD1: 8-pt sinking (NPN) DC output, no status data / no config** →
  `P1-08TD1.md`
- **P1-08TD2: 8-pt sourcing (PNP) DC output, no status data / no config** →
  `P1-08TD2.md`
