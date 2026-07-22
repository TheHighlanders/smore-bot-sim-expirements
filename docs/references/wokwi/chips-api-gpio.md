> Offline reference snapshot for citation. Source: https://docs.wokwi.com/chips-api/gpio
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Chips API — GPIO Pins API

## Function signatures

```c
pin_t    pin_init(const char *name, uint32_t mode);
void     pin_mode(pin_t pin, uint32_t mode);
void     pin_write(pin_t pin, uint32_t value);
uint32_t pin_read(pin_t pin);
bool     pin_watch(pin_t pin, pin_watch_config_t *config);
void     pin_watch_stop(pin_t pin);
```

- **`pin_init(name, mode)`** — resolves a chip pin by name and returns a
  `pin_t`. **Can only be called from `chip_init()`; do not call it later.**
- **`pin_mode`** — change a pin's mode after init.
- **`pin_write` / `pin_read`** — drive / sample a digital pin.
- **`pin_watch`** — register a callback fired on pin transitions.
- **`pin_watch_stop`** — remove a previously registered watch.

## Pin mode constants

`INPUT`, `INPUT_PULLUP`, `INPUT_PULLDOWN`, `OUTPUT`, `OUTPUT_LOW`,
`OUTPUT_HIGH`, `ANALOG`.

## Pin value constants

`LOW`, `HIGH`.

## Watch edge constants

- `RISING` — LOW → HIGH transitions
- `FALLING` — HIGH → LOW transitions
- `BOTH` — any value change

## Types

- `pin_t` — pin handle type.
- `NO_PIN` — sentinel used by device configs to indicate "no pin connected".

## `pin_watch_config_t`

```c
typedef struct {
  uint32_t edge;                                              // RISING | FALLING | BOTH
  void (*pin_change)(void *user_data, pin_t pin, uint32_t value);
  void *user_data;
} pin_watch_config_t;
```

Callback signature:

```c
void chip_pin_change(void *user_data, pin_t pin, uint32_t value);
```

## Example

```c
const pin_watch_config_t watch_config = {
  .edge = FALLING,
  .pin_change = chip_pin_change,
  .user_data = chip,
};
pin_watch(pin, &watch_config);
```
