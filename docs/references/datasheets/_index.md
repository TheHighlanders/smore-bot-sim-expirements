# Datasheet references — index

Offline snapshots of AutomationDirect / FACTS datasheets for citation in the
p1am-wokwi-lab educational project. Retrieved: **2026-07-22**.

Each entry pairs the original PDF (verbatim, for citation) with a machine-readable
`.md` summary of the key electrical specs.

| Name | Description | PDF | Markdown | Source URL | Text extraction |
|------|-------------|-----|----------|------------|-----------------|
| p1amgpio | P1AM-GPIO header pin breakout shield | [p1amgpio.pdf](p1amgpio.pdf) (544 KB) | [p1amgpio.md](p1amgpio.md) | https://cdn.automationdirect.com/static/specs/p1amgpio.pdf | ✅ clean (1 page) |
| P1-08TRS | 8-point isolated relay output module | [P1-08TRS.pdf](P1-08TRS.pdf) (2.4 MB) | [P1-08TRS.md](P1-08TRS.md) | https://cdn.automationdirect.com/static/specs/P1-08TRS.pdf | ✅ clean (4 pages) |
| P1-08TD1 | 8-point sinking DC output module | [P1-08TD1.pdf](P1-08TD1.pdf) (2.4 MB) | [P1-08TD1.md](P1-08TD1.md) | https://cdn.automationdirect.com/static/specs/P1-08TD1.pdf | ✅ clean (4 pages) |

## Quick spec comparison (the two output modules)

| | P1-08TRS | P1-08TD1 |
|--|----------|----------|
| Output type | Relay (6× FORM A SPST, 2× FORM C SPDT) | Sinking, N-channel MOSFET open drain |
| Channels | 8 | 8 |
| Per-point rating | 3 A (2 A via ZIPLink) | 1 A |
| Voltage | 6–24 VDC / 6–120 VAC (op. 5.1–30 VDC / 5.1–132 VAC) | 3.3–24 VDC (op. 2.9–26.4 VDC) |
| Commons | 8 isolated (1/common) | 1 non-isolated |
| Max fuse | 8 A | 8 A |
| Terminal block | 18-pos (P2-RTB / P2-RTB-1) | 10-pos (P1-10RTB / P1-10RTB-1) |

Notes:
- All three PDFs downloaded successfully and verified as real PDFs (> 5 KB); no 404s or HTML.
- Text extraction with pypdf succeeded for all three — no FlateDecode empty-text issues.
- The P1AM-GPIO 1-page spec does not publish ESD levels, PTC fuse trip/hold ratings,
  or a named unprotected pin; see `p1amgpio.md` for the caveat and the protection
  description that is available.
