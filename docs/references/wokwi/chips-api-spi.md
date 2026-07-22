> Offline reference snapshot for citation. Source: https://docs.wokwi.com/chips-api/spi
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Chips API — SPI Device API

Implements an SPI peripheral (device/slave) inside a custom chip.

## Function signatures

```c
spi_dev_t spi_init(spi_config_t *config);
void      spi_start(spi_dev_t spi, uint8_t *buffer, uint32_t count);
void      spi_stop(spi_dev_t spi);
```

- **`spi_init(config)`** — creates the SPI device from a `spi_config_t`.
  **Can only be called from `chip_init()`; do not call it at a later time.**
- **`spi_start(spi, buffer, count)`** — begins a transaction, sending/receiving
  up to `count` bytes through `buffer`.
- **`spi_stop(spi)`** — stops the transfer; triggers the `done` callback with
  the number of bytes actually transferred.

## `spi_config_t` fields

| Field | Type | Purpose |
|-------|------|---------|
| `sck` | `pin_t` | SPI clock pin |
| `mosi` | `pin_t` | Master-Out-Slave-In data pin (use `NO_PIN` to disable) |
| `miso` | `pin_t` | Master-In-Slave-Out data pin (use `NO_PIN` to disable) |
| `mode` | `uint32_t` | SPI mode 0, 1, 2, or 3 (default 0) |
| `done` | callback ptr | Called when the transfer buffer fills or `spi_stop()` is called |
| `user_data` | `void *` | Opaque pointer passed back to the `done` callback |

## `done` callback signature

```c
void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count);
```

`buffer` holds the received data; `count` is the number of bytes transferred.

## Chip-select (CS) is NOT handled by the SPI API

The SPI API does **not** manage the chip-select line. The chip author must
watch the CS pin manually with **`pin_watch`** (from the GPIO API) and, in the
pin-change callback, call `spi_start()` when the device is selected and
`spi_stop()` when it is deselected.
