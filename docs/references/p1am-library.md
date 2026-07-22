# Reference: real P1AM hardware behaviour (authoritative)

Offline citation source for every claim this project makes about how the **real**
FACTS Engineering P1AM controller and its modules behave. All facts below are
quoted from the P1AM Arduino library, which is the authority for the wire
protocol, pin map, opcodes, and module catalog.

- **Upstream:** https://github.com/facts-engineering/P1AM  (library **v1.0.10**)
- **Also:** https://github.com/facts-engineering/P1AMCore  (SAMD core / pin variants)
- **Retrieved:** 2026-07-22
- **License:** MIT © 2023 FACTS Engineering, LLC — see [p1am-library/LICENSE](p1am-library/LICENSE)
- **Local snapshots** (MIT, copied verbatim for offline use):
  [defines.h](p1am-library/defines.h) ·
  [Module_List.h](p1am-library/Module_List.h) ·
  [P1AM.h](p1am-library/P1AM.h) ·
  [README.md](p1am-library/README.md)

> Line numbers below refer to the local snapshots (= upstream v1.0.10). The
> `P1AM.cpp` implementation file is large and not copied; its cited lines are
> quoted inline here and can be viewed upstream at
> `github.com/facts-engineering/P1AM/blob/master/src/P1AM.cpp`.

---

## <a id="spi-bus"></a>SPI bus: mode 2, 1 MHz, MSB-first

> `SPISettings P100_SPI_SETTINGS(1000000, MSBFIRST, SPI_MODE2);`
> — `src/P1AM.cpp:27`

The CPU is the SPI controller; the Base Controller is the peripheral. 1 MHz
clock, MSB-first, **SPI mode 2** (CPOL=1, CPHA=0).

## <a id="spi-bus-pins"></a>SPI bus pins (P1AM-100 / SAMD21)

From [p1am-library/README.md](p1am-library/README.md) (lines 28–36) and
`P1AMCore/variants/P1AM-100/variant.h:94-96`:

| Signal | Arduino pin | variant.h |
|--------|-------------|-----------|
| MOSI   | D8          | `PIN_SPI_MOSI (8u)` |
| CLK/SCK| D9          | `PIN_SPI_SCK  (9u)` |
| MISO   | D10         | `PIN_SPI_MISO (10u)`|

> "The **P1AM-100** communications with the SAMD21 are SPI based and use 5 total
> pins. Pins 8, 9 and 10 can be shared… **A3 and A4 must not be used** on any
> shield if using the base controller" — README.md:28

## <a id="control-pins"></a>Control / handshake pins

> `#define slaveSelectPin  A3`  · `#define slaveAckPin  A4`
> `#define SWITCH_BUILTIN  31`  · `#define baseEnable   33`  · `#define _P1AM_SPI  SPI`
> — [defines.h:47-51](p1am-library/defines.h) (P1AM-100 / SAMD21)

The P1AM-200 / SAMD51 variant uses `44 / 47 / 31 / 46 / SPI2` — [defines.h:40-44](p1am-library/defines.h).

| Role | P1AM-100 pin |
|------|--------------|
| Chip select (CS) | A3 |
| ACK (base → CPU) | A4 |
| Base enable (out)| D33 |

## <a id="opcodes"></a>Command opcodes (headers)

From [defines.h:56-77](p1am-library/defines.h):

| Opcode | Value | | Opcode | Value |
|--------|-------|-|--------|-------|
| `MOD_HDR` | 0x02 | | `READ_DISCRETE_HDR` | 0x50 |
| `VERSION_HDR` | 0x03 | | `READ_ANALOG_HDR` | 0x51 |
| `ACTIVE_HDR` | 0x04 | | `READ_BLOCK_HDR` | 0x52 |
| `DROPOUT_HDR` | 0x05 | | `WRITE_DISCRETE_HDR` | 0x60 |
| `CFG_HDR` | 0x10 | | `WRITE_ANALOG_HDR` | 0x61 |
| `READ_CFG_HDR` | 0x11 | | `WRITE_BLOCK_HDR` | 0x62 |
| `READ_STATUS_HDR` | 0x40 | | `FW_UPDATE_HDR` | 0xAA |
| `DUMMY` | 0xFF | | `MAX_TIMEOUT` | 0xFFFFFFFF |

This project implements the subset `0x02`, `0x03`, `0x50`, `0x60`.

## <a id="ack-handshake"></a>ACK handshake: 3-edge pulse per command

`dataSync()` (called after nearly every command) waits for the ACK line to go
through **three edges**: high → low → high, each with a 200 ms timeout.

> ```
> while(!digitalRead(slaveAckPin)){ ... }   // 1) wait HIGH
> delayMicroseconds(1);
> while( digitalRead(slaveAckPin)){ ... }    // 2) wait LOW
> delayMicroseconds(1);
> while(!digitalRead(slaveAckPin)){ ... }   // 3) wait HIGH
> ```
> — `src/P1AM.cpp:1296-1331` (`void P1AM::dataSync()`)

## <a id="discrete-api"></a>Discrete API (signatures + channel-0 convention)

From [P1AM.h:45-51](p1am-library/P1AM.h):

> `uint8_t init();` — "Returns the number of slots that have signed on."
> `uint32_t readDiscrete(uint8_t slot, uint8_t channel = 0);` — "Passing 0
> instead of a channel will return data from all of the channels at once."
> `void writeDiscrete(uint32_t data, uint8_t slot, uint8_t channel = 0);` —
> "Passing 0 instead of a channel will write data for all of the channels at once."

Slots and channels are **1-based**; `channel == 0` = whole-module bitmap. (The
online API reference states LSB = channel 1; see
[facts-docs/api_reference.md](facts-docs/api_reference.md).)

## <a id="discrete-data-model"></a>Module data model (`moduleProps`)

From [Module_List.h:28-38](p1am-library/Module_List.h):

> ```
> struct moduleProps {
>   unsigned int moduleID;   // 32-bit ID reported during enumeration
>   char diBytes;            // discrete input bytes
>   char doBytes;            // discrete output bytes
>   char aiBytes, aoBytes;   // analog in/out bytes
>   char statusBytes;        // e.g. overrange / missing 24V
>   char configBytes;        // configuration bytes
>   char dataSize;           // resolution / specialty
>   const char* moduleName;
> };
> ```

Discrete: 1 bit per channel, 8 channels/byte, **channel 1 = LSB**. 8-ch module ⇒
`doBytes = 1`; 16-ch ⇒ `doBytes = 2`.

## <a id="module-ids"></a>Module IDs (the ones this project models)

Verbatim rows from [Module_List.h](p1am-library/Module_List.h)
(`{id, di, do, ai, ao, status, config, dataSize, name}`):

| Module | ID | di | do | Line |
|--------|----|----|----|------|
| P1-08ND3 | `0x04A00081` | 1 | 0 | Module_List.h:46 |
| P1-08TRS | `0x1404008F` | 0 | 1 | Module_List.h:66 |
| P1-16TR  | `0x14040091` | 0 | 2 | Module_List.h:68 |
| P1-08TD1 | `0x14050081` | 0 | 1 | Module_List.h:72 |
| P1-08TD2 | `0x14050082` | 0 | 1 | Module_List.h:74 |
| P1-16CDR | `0x24A50081` | 1 | 1 | Module_List.h:80 |

> ⚠️ History note: an earlier draft of `shared/module_db.h` had the wrong IDs for
> P1-08TD1/P1-08TD2 (`0x1400008D`/`0x1400408E`). The authoritative values above
> are `0x14050081`/`0x14050082`. This is why claims get cited.
