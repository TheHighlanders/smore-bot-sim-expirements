# P1-08TRS — 8-Point Isolated Relay Output Module

> Offline reference snapshot for citation. Source: https://cdn.automationdirect.com/static/specs/P1-08TRS.pdf
> Retrieved: 2026-07-22. Full datasheet saved alongside as `P1-08TRS.pdf`.
> Text layer extracted cleanly from the PDF (4 pages). Document: P1-08TRS-DS, 3rd Edition, 9/17/2019.

## Overview

The P1-08TRS is a high-current **isolated relay** output module providing eight
3 A surge-protected outputs for the Productivity1000 system. It offers both
normally open (NO) and normally closed (NC) relay contacts.

- Family: Productivity1000
- Module location: any I/O position in a Productivity1000 System
- Agency approvals: UL 61010-1 and UL 61010-2-201 (File E139594, Canada & USA);
  CE (EN 61131-2 EMC, EN 61010-1, EN 61010-2-201 Safety)

## Key output specifications

| Spec | Value |
|------|-------|
| Output type | **Relay** — 6 relays FORM A (SPST) + 2 relays FORM C (SPDT) |
| Outputs per module | **8** |
| Commons | **8 isolated** (1 point per common) |
| Rated voltage | 6–24 VDC / 6–120 VAC |
| Operating voltage range | 5.1–30 VDC / 5.1–132 VAC |
| AC frequency | 47–63 Hz |
| Max output current | **3 A / point** (both AC and DC); **2 A / point** if used with ZIPLink cable |
| Minimum load current | 5 mA @ 5 VDC |
| Maximum inrush current | 3 A for 10 ms |
| OFF→ON response | < 10 ms |
| ON→OFF response | < 10 ms |
| Status indicators | Logic side (8 points) |
| Maximum applicable fuse | 8 A (user-supplied external fuse) |

### Typical relay life (operations at 4 A load current)

| Voltage & load type | Operations |
|---------------------|-----------|
| 30 VDC resistive | 100,000 |
| 30 VDC solenoid | 100,000 |
| 120 VAC resistive | 100,000 |
| 120 VAC solenoid | 100,000 |

## Contact / terminal layout

Relay contacts brought to the 18-position terminal block:

- **FORM C (SPDT)** relays on channels 1 and 5: `C1 / NO1 / NC1` and `C5 / NO5 / NC5`
  (each has a common, a normally-open, and a normally-closed terminal).
- **FORM A (SPST, NO only)** relays on channels 2, 3, 4, 6, 7, 8:
  `C2/NO2`, `C3/NO3`, `C4/NO4`, `C6/NO6`, `C7/NO7`, `C8/NO8`.
- Schematic shows each output fed through the switched contact to the load, with an
  **8 A user-supplied external fuse** in the field supply (6–30 VDC / 6–120 VAC, 50–60 Hz).

## Terminal block specifications (connector sold separately)

| Part number | P2-RTB | P2-RTB-1 |
|-------------|--------|----------|
| Type | 18 screw terminals | 18 spring-clamp terminals |
| Wire range | 30–16 AWG (0.051–1.31 mm²) | 28–16 AWG (0.081–1.31 mm²) |
| Insulation max | 3/64 in (1.2 mm) | 3/64 in (1.2 mm) |
| Strip length | 1/4 in (6–7 mm) | 19/64 in (7–8 mm) |
| Screw size / torque | M2 / 2.5 lb·in (0.28 N·m) | N/A (spring clamp) |
| Conductors | "USE COPPER CONDUCTORS, 75 ºC" or equivalent | same |

## General specifications

| Spec | Value |
|------|-------|
| Operating temperature | 0 to 60 °C (32 to 140 °F) |
| Storage temperature | -20 to 70 °C (-4 to 158 °F) |
| Humidity | 5 to 95 % (non-condensing) |
| Logic isolation | 1800 VAC applied for 1 second |
| Insulation resistance | > 10 MΩ @ 500 VDC |
| Heat dissipation | 3000 mW |
| Enclosure type | Open Equipment |
| Connector | 18-position removable terminal block (sold separately) |
| Weight | 112 g (4 oz) |

## Warning

Do not add or remove modules with field power applied.
