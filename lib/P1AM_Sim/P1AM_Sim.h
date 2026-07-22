/*
 * P1AM_Sim.h  --  a drop-in fake of the FACTS Engineering P1AM library.
 *
 * Students write the SAME code they would on real ProductivityOpen hardware:
 *
 *     #include "P1AM_Sim.h"
 *     void setup() { while (!P1.init()) {} }
 *     void loop()  { P1.writeDiscrete(HIGH, 1, 3); }   // slot 1, channel 3 ON
 *
 * The difference is under the hood: instead of driving a physical base
 * controller, P1AM_Sim talks to an IBaseTransport. Two transports exist:
 *   - SpiTransport  : real SPI to the Wokwi base-controller custom chip
 *                     (compiled in the Arduino / RP2040 build)
 *   - MockTransport : an in-process base_model, no SPI at all
 *                     (compiled in the native host build for unit tests)
 *
 * API surface mirrors the real library: slots are 1-based, channels are
 * 1-based, and channel 0 means "all channels as a bitmap" (LSB = channel 1).
 * [ref: docs/references/p1am-library.md#discrete-api -> P1AM.h:45-51;
 *  LSB=channel-1 detail: docs/references/facts-docs/api_reference.md]
 */
#ifndef P1AM_SIM_H
#define P1AM_SIM_H

#include <stdint.h>
#include <stddef.h>
#include "module_db.h"

#if defined(ARDUINO)
#include <Arduino.h>
#else
/* Host build: provide the couple of Arduino constants students rely on. */
#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW 0
#endif
#endif

/*
 * Transport abstraction. A transport performs one full command exchange:
 * it sends `cmd_len` command bytes and reads back exactly `resp_len` response
 * bytes, hiding all the chip-select / ACK handshaking.
 */
class IBaseTransport {
public:
    virtual ~IBaseTransport() {}
    virtual void begin() = 0;
    virtual void command(const uint8_t *cmd, uint8_t cmd_len,
                         uint8_t *resp, uint8_t resp_len) = 0;
};

class P1AM_Sim {
public:
    explicit P1AM_Sim(IBaseTransport &transport) : _t(transport), _numModules(0) {}

    /* Enumerate the base. Returns the number of modules found (0 == failure,
     * matching the real library's `while(!P1.init())` idiom). */
    uint8_t init();

    /* Number of modules found by the last init(). */
    uint8_t modules() const { return _numModules; }

    /* Part-number string for a slot (1-based), or "UNKNOWN". */
    const char *slotName(uint8_t slot) const;

    /* Write discrete outputs.
     *   channel != 0 : set that single channel to (data & 1)
     *   channel == 0 : `data` is a bitmap written across the module (LSB=ch1) */
    void writeDiscrete(uint32_t data, uint8_t slot, uint8_t channel = 0);

    /* Read discrete image.
     *   channel != 0 : returns 0/1 for that channel
     *   channel == 0 : returns the whole module as a bitmap (LSB=ch1) */
    uint32_t readDiscrete(uint8_t slot, uint8_t channel = 0);

    /* Read one analog input channel as raw counts (not scaled), matching the
     * real API. [ref: docs/references/p1am-library.md -> P1AM.h:52] */
    int readAnalog(uint8_t slot, uint8_t channel);

    /* Read one temperature channel (P1-04THM/NTC) as a float, reinterpreting the
     * same 4 bytes readAnalog returns. [ref: p1am-library.md -> P1AM.h:53] */
    float readTemperature(uint8_t slot, uint8_t channel);

    /* Base controller firmware version. The real library documents this as
     * "byte format X.Y.ZZ" [ref: docs/references/p1am-library.md, P1AM.cpp:1152];
     * the sim returns a fixed raw value (see base_model.h bm_init). */
    uint32_t getFwVersion();

private:
    IBaseTransport &_t;
    uint8_t  _numModules;
    uint32_t _slotId[P1_MAX_SLOTS];   /* module id per slot (index 0 == slot 1) */
    uint8_t  _slotDo[P1_MAX_SLOTS];   /* doBytes per slot                        */
};

/* Global instance, exactly like the real library's `P1`.
 * Defined in P1AM_Sim.cpp (Arduino build) bound to the SPI transport. */
extern P1AM_Sim P1;

#endif /* P1AM_SIM_H */
