> Offline reference snapshot for citation. Source: https://docs.wokwi.com/parts/wokwi-pushbutton
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Part — Pushbutton (`wokwi-pushbutton`)

Part type identifier: **`wokwi-pushbutton`** (12mm tactile momentary push
button; a 6mm variant also exists).

## Pins

| Pin | Meaning |
|-----|---------|
| `1.l` | Contact 1, left side |
| `1.r` | Contact 1, right side |
| `2.l` | Contact 2, left side |
| `2.r` | Contact 2, right side |

**Internal wiring:** the two terminals of the *same* contact are permanently
connected (`1.l`↔`1.r`, and `2.l`↔`2.r`). Pressing the button connects
contact 1 to contact 2, closing the circuit.

## Attributes

| Attribute | Purpose | Default |
|-----------|---------|---------|
| `color` | Button color (named or hex) | `"red"` |
| `label` | Text shown below the button | `""` |
| `key` | Keyboard shortcut to press the button | `""` |
| `bounce` | Set to `"0"` to disable contact-bounce simulation | `""` |
| `xray` | Set to `"1"` to reveal internal wiring | `""` |

## "pressed" automation control

The button exposes an integer control named **`pressed`**: `1` = pressed,
`0` = released.

```yaml
- set-control:
    part-id: btn1
    control: pressed
    value: 1
- delay: 200ms
- set-control:
    part-id: btn1
    control: pressed
    value: 0
```

## Notes

- **Bouncing:** closing the switch makes the contacts separate/reconnect
  ~10–100 times over ~1ms (simulated unless `bounce` is `"0"`).
- **Stickiness:** Ctrl-click (Cmd-click on Mac) toggles a stuck state so
  multiple buttons can be held at once.
