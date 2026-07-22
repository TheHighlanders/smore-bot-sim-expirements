> Offline reference snapshot for citation. Source: https://docs.wokwi.com/chips-api/chip-json
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Chips API — Chip JSON (`<name>.chip.json`)

Describes a custom chip's identity, pinout, and optional UI. Top-level keys:

| Key | Type | Purpose |
|-----|------|---------|
| `name` | string | Chip name shown in the diagram editor |
| `author` | string | Chip author |
| `pins` | array of strings | Ordered list of pin names (see gap convention) |
| `controls` | array of objects | Optional interactive controls (sliders) |
| `display` | object | Optional attached display dimensions |

## `pins` array and the `""` gap convention

`pins` lists pin names in order starting from pin 1. An **empty string `""`**
represents a *skipped / blank pin position* — used to create spacing/gaps so
the physical pin layout lines up with a real package.

```json
"pins": ["VCC", "GND", "RST", "", "SCL", "SDA"]
```

Here position 4 is a gap; the array still occupies 6 physical positions.

## `controls` (each entry)

| Key | Type | Purpose |
|-----|------|---------|
| `id` | string | Control identifier — use `camelCase` |
| `label` | string | User-facing label |
| `type` | string | Control type; `"range"` (slider) is the documented type |
| `min` | number | Minimum value |
| `max` | number | Maximum value |
| `step` | number | Increment |

## `display`

```json
"display": { "width": 128, "height": 64 }
```

`width`/`height` are in pixels and define the render surface used by the
Framebuffer API for custom LCD/OLED/e-paper chips.
