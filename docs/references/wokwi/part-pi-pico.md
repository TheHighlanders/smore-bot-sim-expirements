> Offline reference snapshot for citation. Source: https://docs.wokwi.com/parts/wokwi-pi-pico
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Part — Raspberry Pi Pico (`wokwi-pi-pico`)

Part type identifier: **`wokwi-pi-pico`**.

## Pin names (exact strings used in diagram.json)

### GPIO / digital
- `GP0`, `GP1`, `GP2`, `GP3`, `GP4`, `GP5`, `GP6`, `GP7`, `GP8`, `GP9`, `GP10`,
  `GP11`, `GP12`, `GP13`, `GP14`, `GP15`, `GP16`, `GP17`, `GP18`, `GP19`,
  `GP20`, `GP21`, `GP22` — digital GPIO pins 0–22.
- `GP26` — digital GPIO + analog input (ADC channel 0).
- `GP27` — digital GPIO + analog input (ADC channel 1).
- `GP28` — digital GPIO + analog input (ADC channel 2).

### Ground (eight pins)
- `GND.1`, `GND.2`, `GND.3`, `GND.4`, `GND.5`, `GND.6`, `GND.7`, `GND.8`
  (physical pin numbers 3, 8, 13, 18, 23, 28, 33, 38).

### Power
- `VSYS` — positive power supply.
- `VBUS` — positive power supply.
- `3V3` — positive power supply (3.3 V).

### Special / test points
- `TP4` — digital GPIO pin 23 (not visible in the diagram editor).
- `TP5` — digital GPIO pin 25 + onboard LED (not visible in the diagram editor).

## Onboard LED

Connected to GPIO 25; lit when the pin is driven HIGH. In Arduino code it is
accessed via the `LED_BUILTIN` constant.

## Not available in the simulation

The `3V3_EN`, `RUN`, and `ADC_VREF` pins are **not available in the
simulation**.
