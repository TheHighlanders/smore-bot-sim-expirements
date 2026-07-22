> Offline reference snapshot for citation. Source: https://docs.wokwi.com/chips-api/framebuffer
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi Chips API — Framebuffer API

Used to implement display devices (LCD, OLED, e-paper) in a custom chip.

## Pixel format

Displays use **32 bits per pixel, RGBA**. Total buffer size =
`pixel_width * pixel_height * 4` bytes.

## Function signatures

```c
buffer_t framebuffer_init(uint32_t *pixel_width, uint32_t *pixel_height);
void     buffer_write(buffer_t buffer, uint32_t offset, void *data, uint32_t data_len);
void     buffer_read(buffer_t buffer, uint32_t offset, void *data, uint32_t data_len);
```

- **`framebuffer_init(&w, &h)`** — returns the framebuffer handle and, via the
  out-params, the pixel dimensions (taken from the `display` block in
  `chip.json`). **Can only be called from `chip_init()`; do not call it later.**
- **`buffer_write(buffer, offset, data, data_len)`** — copy `data_len` bytes
  into the framebuffer at byte `offset`.
- **`buffer_read(buffer, offset, data, data_len)`** — copy `data_len` bytes
  out of the framebuffer at byte `offset`.

## Types

- `buffer_t` — opaque framebuffer handle.

## Relationship to chip.json

Display dimensions come from the `"display": { "width": …, "height": … }`
object in the chip's `.chip.json`.

## Example chips referenced on the page

- Basic Framebuffer Chip example
- SSD1306 I2C OLED display
- IL9163 128x128 color LCD display driver
