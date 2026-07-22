> Offline reference snapshot for citation. Source: https://docs.wokwi.com/parts/wokwi-led
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Part — LED (`wokwi-led`)

Part type identifier: **`wokwi-led`** (standard 5mm LED).

## Pins

| Pin | Meaning |
|-----|---------|
| `A` | Anode (positive) |
| `C` | Cathode (negative) |

## Attributes

| Attribute | Purpose | Default |
|-----------|---------|---------|
| `color` | LED body color (named color or hex) | `"red"` |
| `lightColor` | Emitted light color | depends on `color` |
| `label` | Text label below the LED | — |
| `gamma` | Gamma-correction factor for PWM realism | `"2.8"` |
| `flip` | Horizontal flip | `""` |
| `fps` | Update framerate | `"80"` |

## `color` examples (from the page)

```json
{ "color": "green" }
{ "color": "#FFFF00" }
{ "color": "white" }
{ "color": "white", "lightColor": "orange" }
```

Set `"gamma": "1.0"` to disable gamma correction. Rotate a selected LED with
the `R` key or the diagram's rotate property.
