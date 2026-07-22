> Offline reference snapshot for citation. Source: https://docs.wokwi.com/parts/wokwi-relay-module
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Part — Relay Module (`wokwi-relay-module`)

Part type identifier: **`wokwi-relay-module`**.

## Pins

**Control side**
- `VCC` — supply voltage
- `GND` — ground
- `IN` — control signal (e.g. from a microcontroller GPIO)

**Switched side**
- `COM` — common
- `NC` — normally closed
- `NO` — normally open

## Onboard LED

The module has a **green LED that lights up when the relay coil is energized**,
giving a visual indication that the relay is activated.

## Attributes

| Attribute | Purpose | Default |
|-----------|---------|---------|
| `transistor` | Drive polarity: `"npn"` (active-high) or `"pnp"` (active-low) | `"npn"` |

## Switching logic

**NPN (default, active-high):**
- `IN` HIGH / disconnected → `COM` connects to `NC`
- `IN` LOW → `COM` connects to `NO`

**PNP (active-low):**
- `IN` HIGH → `COM` connects to `NO`
- `IN` LOW / disconnected → `COM` connects to `NC`
