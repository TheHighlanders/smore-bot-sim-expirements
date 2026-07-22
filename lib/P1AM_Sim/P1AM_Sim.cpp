/*
 * P1AM_Sim.cpp  --  transport-agnostic implementation of the fake P1AM API.
 *
 * All the interesting logic (command framing, little-endian assembly, channel
 * bit math) lives here and is compiled in BOTH the host-test build and the
 * Arduino build. Only the global `P1` object + its SPI transport are
 * Arduino-only (bottom of file).
 */
#include "P1AM_Sim.h"
#include "base_model.h"   /* opcode + response-length constants */

uint8_t P1AM_Sim::init() {
    uint8_t cmd[1]  = { P1_MOD_HDR };
    uint8_t resp[P1_ENUM_RESP_LEN];
    _t.begin();
    _t.command(cmd, 1, resp, P1_ENUM_RESP_LEN);

    uint8_t count = resp[0];
    if (count > P1_MAX_SLOTS) {
        count = 0;                 /* garbage -> treat as "no base" */
    }
    for (uint8_t s = 0; s < count; s++) {
        uint32_t id = (uint32_t)resp[1 + s * 4 + 0]
                    | ((uint32_t)resp[1 + s * 4 + 1] << 8)
                    | ((uint32_t)resp[1 + s * 4 + 2] << 16)
                    | ((uint32_t)resp[1 + s * 4 + 3] << 24);
        _slotId[s] = id;
        _slotDo[s] = mdb_lookup(id)->doBytes;
    }
    _numModules = count;
    return count;
}

const char *P1AM_Sim::slotName(uint8_t slot) const {
    if (slot < 1 || slot > _numModules) {
        return P1_MODULE_UNKNOWN.name;
    }
    return mdb_lookup(_slotId[slot - 1])->name;
}

void P1AM_Sim::writeDiscrete(uint32_t data, uint8_t slot, uint8_t channel) {
    uint8_t cmd[3 + P1_MAX_DO_BYTES];
    uint8_t status[P1_WRITE_RESP_LEN];
    cmd[0] = P1_WRITE_DISCRETE_HDR;
    cmd[1] = slot;
    cmd[2] = channel;

    uint8_t len;
    if (channel == 0) {
        uint8_t db = (slot >= 1 && slot <= _numModules) ? _slotDo[slot - 1] : 1;
        if (db == 0 || db > P1_MAX_DO_BYTES) db = 1;
        for (uint8_t b = 0; b < db; b++) {
            cmd[3 + b] = (uint8_t)((data >> (8 * b)) & 0xFF);
        }
        len = (uint8_t)(3 + db);
    } else {
        cmd[3] = (uint8_t)(data & 0x01);
        len = 4;
    }
    _t.command(cmd, len, status, P1_WRITE_RESP_LEN);
}

uint32_t P1AM_Sim::readDiscrete(uint8_t slot, uint8_t channel) {
    uint8_t cmd[2] = { P1_READ_DISCRETE_HDR, slot };
    uint8_t resp[P1_READ_RESP_LEN];
    _t.command(cmd, 2, resp, P1_READ_RESP_LEN);

    uint32_t val = (uint32_t)resp[0]
                 | ((uint32_t)resp[1] << 8)
                 | ((uint32_t)resp[2] << 16)
                 | ((uint32_t)resp[3] << 24);
    if (channel == 0) {
        return val;
    }
    return (val >> (channel - 1)) & 0x1UL;
}

int P1AM_Sim::readAnalog(uint8_t slot, uint8_t channel) {
    uint8_t cmd[3] = { P1_READ_ANALOG_HDR, slot, channel };
    uint8_t resp[P1_ANALOG_RESP_LEN];
    _t.command(cmd, 3, resp, P1_ANALOG_RESP_LEN);
    uint32_t v = (uint32_t)resp[0]
               | ((uint32_t)resp[1] << 8)
               | ((uint32_t)resp[2] << 16)
               | ((uint32_t)resp[3] << 24);
    return (int)v;
}

float P1AM_Sim::readTemperature(uint8_t slot, uint8_t channel) {
    /* Same 4 bytes as readAnalog, reinterpreted as an IEEE-754 float (the real
     * library returns temperature as a float). memcpy avoids aliasing UB. */
    uint32_t v = (uint32_t)readAnalog(slot, channel);
    float f;
    memcpy(&f, &v, sizeof(f));
    return f;
}

uint32_t P1AM_Sim::getFwVersion() {
    uint8_t cmd[1] = { P1_VERSION_HDR };
    uint8_t resp[P1_VERSION_RESP_LEN];
    _t.command(cmd, 1, resp, P1_VERSION_RESP_LEN);
    return (uint32_t)resp[0]
         | ((uint32_t)resp[1] << 8)
         | ((uint32_t)resp[2] << 16)
         | ((uint32_t)resp[3] << 24);
}

/* ---------------------------------------------------------------------------
 * Arduino/RP2040 build only: the global `P1` bound to the real SPI transport.
 * The host test build supplies its own P1AM_Sim + MockTransport, so none of
 * this (and no <SPI.h>) is compiled there.
 * ------------------------------------------------------------------------- */
#if defined(ARDUINO)
#include "SpiTransport.h"
static SpiTransport g_spiTransport;
P1AM_Sim P1(g_spiTransport);
#endif
