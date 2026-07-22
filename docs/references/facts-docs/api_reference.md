> Offline reference snapshot for citation. Source: https://facts-engineering.github.io/api_reference.html
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# P1AM Arduino Library API Reference — Key Technical Facts

## Initialization

### `uint8_t P1.init()`
- **Purpose**: Initializes P1AM communications to P1 modules; must be called before other module functions.
- **Parameters**: None
- **Returns**: `0-15` — the number of P1 modules found connected to the P1AM.

---

## Discrete I/O Functions

### `uint32_t P1.readDiscrete(uint8_t slot, uint8_t channel = 0)`
- **Parameters**:
  - `uint8_t slot` — slot number 1-15 of the discrete input module
  - `uint8_t channel` — specific channel to read (default `0`)
- **Returns**: 32-bit value (`0` or `1` for a specific channel; bitmapped integer for all channels when `channel = 0`)
- **Note**: When the channel parameter equals `0`, each bit of the returned value corresponds to the discrete input status for that channel, with the **LSB representing Channel 1**.

### `void P1.writeDiscrete(uint32_t data, uint8_t slot, uint8_t channel = 0)`
- **Parameters** (argument order = data, slot, channel):
  - `uint32_t data` — value to write (`0` or `1` for a specific point; bitmapped integer for all points)
  - `uint8_t slot` — slot number 1-15 of the discrete output module
  - `uint8_t channel` — specific channel (default `0` writes to all channels, bitmapped, LSB = Channel 1)
- **Returns**: None

---

## Analog I/O Functions

### `int P1.readAnalog(uint8_t slot, uint8_t channel)`
- **Parameters**:
  - `uint8_t slot` — slot number 1-15 of the analog input module
  - `uint8_t channel` — specific channel (channels start at 1)
- **Returns**: 32-bit integer value
  - 12-bit modules: `0–4095`
  - 16-bit modules: `0–65535` or `−32768–32767`

### `float P1.readTemperature(uint8_t slot, uint8_t channel)`
- **Parameters**:
  - `uint8_t slot` — slot number 1-15 of the temperature input module
  - `uint8_t channel` — specific channel (channels start at 1)
- **Returns**: 32-bit floating-point value in degrees or volts

### `void P1.writeAnalog(uint32_t data, uint8_t slot, uint8_t channel)`
- **Parameters** (argument order = data, slot, channel):
  - `uint32_t data` — value to write (range `0–4095` for 12-bit; `0–65535` for 16-bit)
  - `uint8_t slot` — slot number 1-15 of the analog output module
  - `uint8_t channel` — specific channel (channels start at 1)
- **Returns**: None

---

## PWM Functions

### `void P1.writePWM(float duty, uint32_t freq, uint8_t slot, uint8_t channel)`
- `float duty` — duty cycle `0.00–100.00`
- `uint32_t freq` — frequency `0–20,000` Hz
- `uint8_t slot` — slot number 1-15
- `uint8_t channel` — channel (channels start at 1)

### `void P1.writePWMDuty(float duty, uint8_t slot, uint8_t channel)`
### `void P1.writePWMFreq(uint32_t freq, uint8_t slot, uint8_t channel)`
### `void P1.writePWMDir(bool data, uint8_t slot, uint8_t channel)`
- `bool data` — direction control (`0` = low; `1` = high)

---

## Status & Status-Check Functions

### `uint8_t P1.checkUnderRange(uint8_t slot, uint8_t channel = 0)`
### `uint8_t P1.checkOverRange(uint8_t slot, uint8_t channel = 0)`
### `uint8_t P1.checkBurnout(uint8_t slot, uint8_t channel = 0)`
- **Parameters**: `slot` (1-15); `channel` (specific channel, or `0` for all — default `0`)
- **Returns**: 8-bit integer (`0`/`1` for a single channel; bitmapped with **LSB = Channel 1** when `channel = 0`)

### `uint8_t P1.check24V(uint8_t slot)`
- **Parameters**: `slot` — slot number 1-15
- **Returns**: 8-bit integer (`0` = 24 VDC present; `1` = 24 VDC missing)

### `char P1.readStatus(int byteNum, int slot)`
- **Parameters**: `int byteNum` — byte offset (starts at 0); `int slot` — slot number 1-15
- **Returns**: single status byte as `char`

### `void P1.readStatus(char buf[], uint8_t slot)` (overload)
- **Parameters**: `char buf[]` — array to store all status bytes; `uint8_t slot` — slot number 1-15
- **Returns**: None

---

## Module Configuration

### `bool P1.configureModule(char cfgData[], uint8_t slot)`
### `bool P1.configureModule(const char cfgData[], uint8_t slot)` (overload)
- **Parameters**: `cfgData[]` — byte array with configuration data; `slot` — slot number 1-15
- **Returns**: Boolean `1` for successful configuration

### `void P1.readModuleConfig(char cfgData[], uint8_t slot)`
- **Parameters**: `cfgData[]` — array to store current module configuration; `slot` — 1-15
- **Returns**: None

---

## Block Data Functions

### `void P1.readBlockData(char *buf, uint16_t len, uint16_t offset, uint8_t type)`
### `void P1.writeBlockData(char *buf, uint16_t len, uint16_t offset, uint8_t type)`
- **Parameters**:
  - `char *buf` — data array (holds requested / to-write values)
  - `uint16_t len` — length of data (max 1200 bytes)
  - `uint16_t offset` — starting byte in the buffer
  - `uint8_t type` — data block type (`DISCRETE_IN_BLOCK`, `ANALOG_IN_BLOCK`, `DISCRETE_OUT_BLOCK`, `ANALOG_OUT_BLOCK`, `STATUS_IN_BLOCK`)
- **Returns**: None

---

## Watchdog Functions

### `void P1.configWD(uint16_t milliseconds, uint8_t toggle)`
- `uint16_t milliseconds` — watchdog timeout value
- `uint8_t toggle` — behavior (`1` = reset after 5000ms; `0` = hold until power cycle)

### `void P1.startWD()`
### `void P1.stopWD()`
### `void P1.petWD()`
- Resets the watchdog timer to prevent timeout if I/O functions are not called frequently.

---

## System Functions

### `uint32_t P1.getFwVersion()`
- **Returns**: 32-bit firmware version (format X.Y.ZZ)

### `bool P1.isBaseActive()`
- **Returns**: Boolean `1` if the P1 Base Controller is active

### `uint8_t P1.printModules()`
- Outputs discovered P1 modules to the serial monitor. Returns number of modules found.

### `uint8_t P1.checkConnection(uint8_t numberOfModules = 0)`
- **Parameters**: `numberOfModules` — expected slot count (default `0`)
- **Returns**: `0` if OK; problematic slot number if not OK; module count from last `init()` if parameter is `0`

### `uint16_t P1.rollCall(const char* moduleNames[], uint8_t numberOfModules)`
- **Parameters**: `moduleNames[]` — array of expected module names (e.g., `{"P1-08SIM", "P1-08TRS"}`); `numberOfModules` — number of expected modules
- **Returns**: 16-bit binary-encoded value (each bit = error/no-error for the corresponding slot; `0` = no errors)

### `void P1.enableBaseController(bool state)`
- **Parameters**: `bool state` — `false` disables the Base Controller
- **Note**: To re-enable, call `P1.init()` (not `enableBaseController(true)`)

---

## Channel Labels Structure

Alternative API using the `channelLabel` struct for readable code:

```cpp
channelLabel highLevelSensor_1 = {1, 2};  // slot 1, channel 2
bool sensorState = P1.readDiscrete(highLevelSensor_1);
```

Supported overloads accept a `channelLabel` in place of the `slot, channel` pair: `readDiscrete`, `writeDiscrete`, `readAnalog`, `readTemperature`, `writeAnalog`, `writePWM`, `writePWMDuty`, `writePWMFreq`, `writePWMDir`, `checkUnderRange`, `checkOverRange`, `checkBurnout`.

---

## Key Conventions (summary)

| Aspect | Convention |
|--------|-----------|
| Slot numbering | 1-based (1–15) |
| Channel numbering | 1-based (channels start at 1) |
| Channel `0` meaning | All channels on a module, returned/written as a bitmapped integer |
| Bitmapped LSB | LSB = Channel 1 |
| `init()` return value | Module count (0–15) |
