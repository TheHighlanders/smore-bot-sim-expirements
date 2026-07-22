> Offline reference snapshot for citation. Source: https://docs.wokwi.com/vscode/project-config
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi VS Code Extension — Project Configuration (`wokwi.toml`)

A Wokwi project needs two files at its root: **`wokwi.toml`** (configuration)
and **`diagram.json`** (the circuit).

## `[wokwi]` section keys

| Key | Purpose |
|-----|---------|
| `version` | Config format version — `version = 1` |
| `firmware` | Path to the compiled firmware (relative to project root) |
| `elf` | (optional) Path to the ELF file; enables debug symbols / optimization |
| `gdbServerPort` | (optional) TCP port for the GDB server used by the debugger |
| `rfc2217ServerPort` | (optional) RFC2217 TCP port for external serial access |
| `vcdFile` | (optional) Logic-analyzer VCD output path (default `wokwi.vcd`) |

Example:

```toml
[wokwi]
version = 1
firmware = 'build/app.hex'
elf = 'build/app.elf'
gdbServerPort = 3333
```

## Firmware formats accepted (by board)

| Board | Formats |
|-------|---------|
| Arduino Uno/Mega, ATtiny85 | `.hex`, `.elf` |
| Raspberry Pi Pico | `.hex`, `.uf2`, `.elf` |
| ESP32 family | `.bin`, `.uf2`, `.elf`, `flasher_args.json` |
| STM32 family | `.hex`, `.bin`, `.elf` |

## Custom chips — `[[chip]]` table array

Each custom chip binary is registered with a `[[chip]]` entry:

```toml
[[chip]]
name = 'inverter'
binary = 'chips/inverter.chip.wasm'
```

- `name` — the chip name (matches the chip in the diagram).
- `binary` — path to the compiled `.chip.wasm`.

## Network port forwarding (ESP32 WiFi) — `[[net.forward]]`

```toml
[[net.forward]]
from = "localhost:8180"
to = "target:80"
```

## Path convention

Always use forward slashes (`/`) in paths for cross-platform compatibility;
avoid backslashes.
