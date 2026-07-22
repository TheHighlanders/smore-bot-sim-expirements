> Offline reference snapshot for citation. Source: https://docs.wokwi.com/wokwi-ci/automation-scenarios
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi CI — Automation Scenarios

Automation scenarios are **YAML** files describing a sequence of actions the
simulator performs (press buttons, change sensor state, check serial/pins).
Load one with the `--scenario` CLI option. (The page notes the scenario API is
currently in **alpha** and may change.)

## File structure

```yaml
name: 'Your scenario name'
version: 1
author: 'Your name'
steps:
  # List of steps:
  - set-control:
      part-id: btn1
      control: pressed
      value: 1
  - delay: 500ms
  - wait-serial: 'Button 1 pressed'
```

## Available step types

### `delay` — wait for an amount of time
`value` with required units (e.g. `200ms`, `30ms`, `2s`).
```yaml
delay: 30ms
```

### `expect-pin` — assert a pin value
Parameters: `part-id`, `pin`, `expected`.
```yaml
expect-pin:
  part-id: esp
  pin: 2
  expected: 1
```

### `set-control` — set a controllable aspect of a part
Parameters: `part-id`, `control`, `value`.
```yaml
set-control:
  part-id: dht
  control: humidity
  value: 39
```

### `wait-serial` — wait for matching serial output
Parameter: the string to match.
```yaml
wait-serial: 'Ready for testing!'
```

### `write-serial` — write to the serial console
Parameter: `value` (a string, or an array of numbers).
```yaml
write-serial: 'Ready for testing!'
write-serial: [87, 111, 107, 119, 105]
```

### `take-screenshot` — capture a component and compare
Parameters: `part-id`, `save-to`, `compare-with`.
```yaml
take-screenshot:
  part-id: 'oled1'
  compare-with: 'screenshots/oled-1.png'
```

### `touch` / `touch-press` / `touch-move` / `touch-release`
Touchscreen input. Parameters: `part-id`, `x`, `y`, optional `duration`,
optional `wait` (release takes `part-id` only).
```yaml
touch:
  part-id: esp32s3box
  x: 120
  y: 160
  duration: 100ms
```
