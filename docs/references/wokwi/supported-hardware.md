> Offline reference snapshot for citation. Source: https://docs.wokwi.com/getting-started/supported-hardware
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi — Supported Hardware

Wokwi simulates microcontrollers, sensors, displays, and more. Supported CPU
architectures: **ARM, AVR, RISC-V, and Xtensa**.

## Supported Microcontrollers (verbatim from the page's table)

| Family | Microcontrollers |
|--------|------------------|
| **AVR** | ATmega328P (Arduino Uno, Arduino Nano), ATmega2560 (Arduino Mega), ATtiny85 |
| **ESP32** | Xtensa: ESP32, ESP32-S2, ESP32-S3 — RISC-V: ESP32-C3, ESP32-C5\*, ESP32-C6, ESP32-C61, ESP32-H2, ESP32-P4, ESP32-S31\* |
| **STM32** | STM32C031, STM32L031, STM32F103C8 |
| **Pi Pico** | RP2040 (Raspberry Pi Pico), a dual-core ARM Cortex-M0+ microcontroller |

\* ESP32-P4 support is in **beta**; ESP32-C5 and ESP32-S31 support is in **alpha**.

## Verification of the citation-critical claims

1. **RP2040 / Raspberry Pi Pico — SUPPORTED.** Listed under the "Pi Pico"
   family as "RP2040 (Raspberry Pi Pico), a dual-core ARM Cortex-M0+
   microcontroller."
2. **ESP32 — SUPPORTED.** A dedicated ESP32 family row lists the base ESP32
   plus S2/S3 (Xtensa) and C3/C5/C6/C61/H2/P4/S31 (RISC-V).
3. **SAMD21 / SAMD51 — NOT SUPPORTED.** Neither "SAMD21", "SAMD51", nor any
   Microchip/Atmel SAM D Cortex-M0+/M4 part appears anywhere in the supported
   microcontroller table. (The only ARM Cortex-M0+ part listed is the RP2040.)

## Also present on the page (non-MCU categories, summarized)

The page continues with large tables of supported **Sensors** (e.g. HC-SR04,
DHT22, DS1307 RTC, PIR, NTC, DS18B20, BMP180, MPU6050, photoresistor, MQ2,
HX711, MFRC522), **Input devices** (Pushbutton, Slide switch, DIP Switch 8,
Keypad 4x4, Analog Joystick, Potentiometer, Slide Potentiometer, Rotary Encoder
KY-040), plus displays, output devices, and logic/misc parts. See the sibling
.html for the exhaustive part lists.
