> Offline reference snapshot for citation. Source: https://docs.wokwi.com/vscode/debugging
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi VS Code Extension — Debugging (GDB)

## wokwi.toml setup

Add a GDB server port to the `[wokwi]` section:

```toml
gdbServerPort = 3333
```

## .vscode/launch.json

Create a `cppdbg` launch config (requires the Microsoft C/C++ Tools extension):

- `"type": "cppdbg"`
- `"program"` — path to the firmware **ELF** file
- `"miDebuggerPath"` — GDB executable matching your target architecture
- `"miDebuggerServerAddress": "localhost:3333"`

## Critical startup order — start the simulation FIRST

1. Build the firmware for the target.
2. Press **F1** → **"Wokwi: Start Simulator and Wait for Debugger"**. The
   simulator loads but pauses execution, waiting for the debugger.
3. Press **F5** to attach the debugger.

Warning from the page: "If you start the debugger first, it will fail to connect
to the simulator." Always start the simulator (step 2) before the debugger.

## Architecture-specific GDB (`miDebuggerPath`) examples

| Project | `miDebuggerPath` |
|---------|------------------|
| ESP-IDF | `${command:espIdf.getToolchainGdb}` |
| PlatformIO ESP32 | `${userHome}/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-gdb` |
| Arduino AVR | a recent `avr-gdb` (the v7.8 bundled with the Arduino IDE is incompatible) |

## Troubleshooting

The error `Remote 'g' packet reply is too long` means the GDB binary does not
match the target architecture — point `miDebuggerPath` at the correct GDB.
