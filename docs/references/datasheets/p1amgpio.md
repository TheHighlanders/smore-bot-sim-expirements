# P1AM-GPIO — Header Pin Breakout Module (GPIO Shield)

> Offline reference snapshot for citation. Source: https://cdn.automationdirect.com/static/specs/p1amgpio.pdf
> Retrieved: 2026-07-22. Full datasheet saved alongside as `p1amgpio.pdf`.
> Text layer extracted cleanly from the PDF (1 page).

## Overview

The P1AM-GPIO is a housed Arduino MKR form-factor shield that brings a subset of
the MKR header pins to the front faceplate via an **18-position terminal block**.
These pins include basic over-voltage, under-voltage, and over-current
protection. It connects to the **left side of the P1AM-100 CPU** and most Arduino
MKR form-factor shields.

- Price (list, at retrieval): $61.00
- Agency approvals: UL 61010-1 and UL 61010-2-201 (File E139594, Canada & USA); CE
- Enclosure type: Open Equipment
- Terminal block connector sold separately — recommended: **P2-RTB** or **P2-RTB-1**

## Electrical / signal level

| Spec | Value |
|------|-------|
| I/O logic level | **3.3 V** — do NOT exceed 3.3 V on any I/O pin |
| Max current, any single I/O pin | **7 mA** per pin |
| Max combined current, pins 0, 1, and 4–10 | **46 mA** combined |
| 5 V pin | 5 V supply **output** (do not apply power to it) |
| VCC pin | 3.3 V supply **output** (do not apply power to it) |
| Vin pin | 5 V regulated supply |
| Heat dissipation | 475 mW |

## Terminal-block pin mapping (MKR Expansion Bus pins)

| Pin group / label | Function |
|-------------------|----------|
| GPIO | A0–A6, 0–14 |
| Analog Input Pins | A0–A6 |
| Analog Output Pins | A0 |
| PWM Pins | 0–8, 10, A3, A4 |
| Interrupt Pins | 0, 1, 4–8, A1, A2 |
| 5V | 5 V supply output |
| Vin | 5 V regulated supply |
| VCC | 3.3 V supply output |
| GND | Ground |
| RST | Reset |
| AREF | Analog Input Reference |

## Protection features

- Provides **basic over-voltage, under-voltage, and over-current protection** on the
  broken-out I/O pins (per the datasheet's description).
- **Pins A3, A4, and 8–10 are used for the base controller** — treat these as reserved
  rather than free general-purpose I/O.

> Note: This 1-page spec sheet does **not** publish ESD test levels, PTC fuse
> trip/hold current ratings, or explicitly name an individual pin that lacks
> protection. Those figures are not extractable because they are not present in
> the datasheet; the protection description above is verbatim to the source.
> The reserved base-controller pins (A3, A4, 8–10) are the closest documented
> "handle with care" callout.

## Critical notes (verbatim from datasheet)

- Pins A3, A4, and 8–10 are used for the base controller.
- Do not exceed 46 mA combined from pins 0, 1, and 4–10.
- Do not exceed 3.3 V on any I/O pin.
- Do not exceed 7 mA on any I/O pin.
- Do not apply power to 5V or VCC.
- **WARNING:** Do not add or remove modules with field power applied.

## General specifications

| Spec | Value |
|------|-------|
| Operating temperature | 0 to 60 °C (32 to 140 °F) |
| Storage temperature | -20 to 70 °C (-4 to 158 °F) |
| Humidity | 5 to 95 % (non-condensing) |
| Environmental air | No corrosive gases permitted |
| Vibration | IEC60068-2-6 (Test Fc) |
| Shock | IEC60068-2-27 (Test Ea) |
| Weight | 56 g (2.0 oz) |
| Module location | Connects to the left side of the P1AM-100 CPU |
