> Offline reference snapshot for citation. Source: https://facts-engineering.github.io/modules/P1AM-GPIO/P1AM-GPIO.html
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# P1AM-GPIO — Key Technical Facts

## Device Description

The P1AM-GPIO is an industrial-rated shield that provides a convenient connection from most of the **P1AM-100 GPIO pins** to a front **18-position terminal block** connector. These are plain **Arduino GPIO pins** on the P1AM-100 — this shield is **NOT** the SPI Base Controller path used for Productivity1000 I/O modules.

- **Signal voltage level:** 3.3 V
- **Operating voltage range:** 0–3.3 V
- **Connector type:** 18-position terminal block

## Protection Features

- **All pins except DAC0** include ESD suppression, overvoltage, and overcurrent protection.
- **DAC0 has no electrical protection.**
- **VCC (3.3 V) supply protection:** PTC resettable fuse — **Trip = 420 mA, Hold = 200 mA** — to protect against overloading the 3.3 V supply.

## Pin / Terminal Mapping Table

| Terminal | Shield Pin | Functions |
|----------|-----------|-----------|
| 1 | VCC | 3.3 V supply output |
| 2 | DAC0* | GPIO, Analog input, Analog output |
| 3 | A1 | GPIO, Analog input, Interrupt |
| 4 | A2 | GPIO, Analog input, Interrupt |
| 5 | A5 | GPIO, Analog input |
| 6 | A6 | GPIO, Analog input |
| 7 | 0 | GPIO, PWM, Interrupt |
| 8 | 1 | GPIO, PWM, Interrupt |
| 9 | 2 | GPIO, PWM |
| 10 | 3 | GPIO, PWM |
| 11 | 4 | GPIO, PWM, Interrupt |
| 12 | 6 | GPIO, PWM, Interrupt |
| 13 | 7 | GPIO, PWM, Interrupt |
| 14 | 11 | GPIO, SDA |
| 15 | 12 | GPIO, SCL |
| 16 | 13 | GPIO, RX |
| 17 | 14 | GPIO, TX |
| 18 | GND | Ground |

\* DAC0 (terminal 2) is the single pin without ESD/overvoltage/overcurrent protection.
