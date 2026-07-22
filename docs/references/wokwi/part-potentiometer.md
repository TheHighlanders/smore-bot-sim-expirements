> Offline reference snapshot for citation. Source: https://docs.wokwi.com/parts/wokwi-potentiometer
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Part — Potentiometer (`wokwi-potentiometer`)

Part type identifier: **`wokwi-potentiometer`** (knob-controlled linear
potentiometer / variable resistor).

## Pins

| Pin | Meaning |
|-----|---------|
| `GND` | Ground |
| `SIG` | Wiper output — connect to an analog input pin |
| `VCC` | Supply voltage |

## Attributes

| Attribute | Purpose | Default |
|-----------|---------|---------|
| `value` | Initial position, integer 0–1023 | `"0"` |

## Notes

- Wokwi does **not** do full analog simulation, so `analogRead(SIG)` returns the
  same result even if `GND`/`VCC` are left unconnected.
- Read with `analogRead()` on an Arduino analog pin (A0, A1, …).
- Keyboard: Left/Right = fine, PageUp/PageDown = coarse, Home/End = extremes.

## Automation control

Exposes a control named **`position`**, a float in the range **0.0–1.0**, e.g.:

```yaml
- set-control:
    part-id: pot1
    control: position
    value: 0.5
```
