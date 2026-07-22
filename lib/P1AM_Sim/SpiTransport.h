/*
 * SpiTransport.h  --  real-SPI transport to the Wokwi base-controller chip.
 *
 * Arduino/RP2040 build ONLY (included from P1AM_Sim.cpp behind #if ARDUINO).
 * Implements the master side of the two-phase, ACK-gated protocol described in
 * shared/base_model.h.
 *
 * Default pin map (Raspberry Pi Pico stand-in, matches wokwi/diagram.json and
 * the real P1AM roles):
 *     role   real P1AM-100 pin      Pico stand-in pin
 *     CS     A3                     GP17
 *     SCK    D9                     GP18
 *     MOSI   D8                     GP19
 *     MISO   D10                    GP16
 *     ACK    A4  (base -> CPU)      GP20
 *     EN     D33 (base enable)      GP21
 * The real P1AM-100 pin roles (CS=A3, ACK=A4, EN=D33) are from defines.h:47-51;
 * the bus pins (MOSI=D8, CLK=D9, MISO=D10) from the P1AM README pin table.
 * [ref: docs/references/p1am-library.md#control-pins and #spi-bus-pins]
 * Override any of them with -D P1_PIN_xxx build flags if you rewire the diagram.
 */
#ifndef P1AM_SPI_TRANSPORT_H
#define P1AM_SPI_TRANSPORT_H

#if defined(ARDUINO)

#include <Arduino.h>
#include <SPI.h>
#include "P1AM_Sim.h"

#ifndef P1_PIN_CS
#define P1_PIN_CS   17
#endif
#ifndef P1_PIN_SCK
#define P1_PIN_SCK  18
#endif
#ifndef P1_PIN_MOSI
#define P1_PIN_MOSI 19
#endif
#ifndef P1_PIN_MISO
#define P1_PIN_MISO 16
#endif
#ifndef P1_PIN_ACK
#define P1_PIN_ACK  20
#endif
#ifndef P1_PIN_EN
#define P1_PIN_EN   21
#endif
#ifndef P1_ACK_TIMEOUT_MS
#define P1_ACK_TIMEOUT_MS 200
#endif

class SpiTransport : public IBaseTransport {
public:
    void begin() override {
        pinMode(P1_PIN_CS, OUTPUT);
        digitalWrite(P1_PIN_CS, HIGH);      /* CS idle high (active-low)  */
        pinMode(P1_PIN_ACK, INPUT);
        pinMode(P1_PIN_EN, OUTPUT);
        digitalWrite(P1_PIN_EN, HIGH);      /* enable the base controller */
#if defined(ARDUINO_ARCH_RP2040)
        SPI.setSCK(P1_PIN_SCK);
        SPI.setTX(P1_PIN_MOSI);
        SPI.setRX(P1_PIN_MISO);
#endif
        SPI.begin();
        delay(5);                           /* let the base come alive    */
    }

    void command(const uint8_t *cmd, uint8_t cmd_len,
                 uint8_t *resp, uint8_t resp_len) override {
        /* NB: real P1AM uses SPI mode 2 @ 1 MHz, MSB-first; the sim chip uses
         * mode 0. [ref: docs/references/p1am-library.md#spi-bus -> P1AM.cpp:27] */
        SPISettings settings(1000000, MSBFIRST, SPI_MODE0);

        waitAck(LOW);                       /* bus idle                   */

        /* Phase 1: send the command frame. */
        SPI.beginTransaction(settings);
        digitalWrite(P1_PIN_CS, LOW);
        for (uint8_t i = 0; i < cmd_len; i++) {
            SPI.transfer(cmd[i]);
        }
        digitalWrite(P1_PIN_CS, HIGH);
        SPI.endTransaction();

        waitAck(HIGH);                      /* chip: response ready       */

        /* Phase 2: clock the fixed-length response back. */
        SPI.beginTransaction(settings);
        digitalWrite(P1_PIN_CS, LOW);
        for (uint8_t i = 0; i < resp_len; i++) {
            resp[i] = SPI.transfer(0xFF);
        }
        digitalWrite(P1_PIN_CS, HIGH);
        SPI.endTransaction();

        waitAck(LOW);                       /* chip: transaction complete */
    }

private:
    void waitAck(int level) {
        unsigned long start = millis();
        while (digitalRead(P1_PIN_ACK) != level) {
            if (millis() - start > P1_ACK_TIMEOUT_MS) {
                return;                     /* time out rather than hang   */
            }
        }
    }
};

#endif /* ARDUINO */
#endif /* P1AM_SPI_TRANSPORT_H */
