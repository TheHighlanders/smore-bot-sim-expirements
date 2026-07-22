> Offline reference snapshot for citation. Source: https://docs.wokwi.com/chips-api/getting-started
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Custom Chips API — Getting Started

## Project structure

A custom chip consists of two files:

- **`chip.json`** — defines the chip's `name`, `author`, and pinout (`pins`),
  plus optional `controls` / `display`. (Full schema on the Chip JSON page.)
- **`chip.c`** — the chip implementation in C.

Create one via the blue **"+"** button in the diagram editor → **"Custom Chip"**,
then specify a name and language.

## Supported implementation languages

- **C** (recommended)
- **Rust**
- **AssemblyScript**
- **Verilog** (experimental)

## Required entry point

```c
void chip_init(void);
```

`chip_init()` is called **for every instance** of the chip placed in the
diagram. Use it to allocate/initialize chip state, configure timers, set up the
various device interfaces, and register pin watches.

Note (repeated across the API pages): the `*_init` calls — `pin_init()`,
`spi_init()`, `framebuffer_init()`, etc. — **can only be called from
`chip_init()`**; do not call them later.

## Chip state

Chips typically define their own `chip_state_t` struct to hold per-instance
state. A pointer to it is stashed in the `user_data` field of the various config
structs (`i2c_config_t`, `spi_config_t`, `timer_config_t`, `pin_watch_config_t`,
…) so it is passed back into each callback.

## Available APIs (each has its own doc page)

- GPIO pins API
- Analog API
- Time / simulation-time API
- UART API
- I2C Device API
- SPI Device API
- Attributes
- Framebuffer API

## Debugging with printf

Standard C `printf()` is supported (requires `#include <stdio.h>`). Output
appears in the **"Chips Console"** tab, and a line is only shown once a newline
(`\n`) is emitted.
