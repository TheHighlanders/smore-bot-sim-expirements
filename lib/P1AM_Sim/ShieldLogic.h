/*
 * ShieldLogic.h  --  pure (Arduino-free) helper logic for the GPIO shield demo.
 *
 * Keeping the arithmetic out of the .ino means it can be unit-tested on the
 * host (see test/test_gpio_logic) without a simulator or a board. The demo
 * sketch (src/main.cpp) calls these; the real P1AM-GPIO shield is plain Arduino
 * GPIO (3.3 V terminal block), NOT the SPI base controller, so there is no
 * base-controller/SPI involvement here.
 * [ref: docs/references/facts-docs/P1AM-GPIO.md]
 */
#ifndef P1AM_SHIELD_LOGIC_H
#define P1AM_SHIELD_LOGIC_H

#include <stdint.h>

namespace shield {

/* Scale a raw ADC reading (0..adcMax) to an 8-bit PWM duty (0..255). */
inline uint8_t potToPwm(uint16_t adc, uint16_t adcMax = 1023) {
    if (adcMax == 0) return 0;
    if (adc > adcMax) adc = adcMax;
    return (uint8_t)(((uint32_t)adc * 255u) / adcMax);
}

/* One-hot "walking" bitmap for a relay chase: bit (step % channels) set.
 * LSB == channel 1, matching writeDiscrete()'s channel-0 bitmap convention. */
inline uint32_t chaseMask(uint8_t step, uint8_t channels) {
    if (channels == 0) return 0;
    return (uint32_t)1u << (step % channels);
}

} // namespace shield

#endif /* P1AM_SHIELD_LOGIC_H */
