/*
 * MockTransport.h  --  in-process base controller for host unit tests.
 *
 * No SPI, no Wokwi, no Arduino: this transport routes each command straight
 * into the SAME base_model.h logic that the Wokwi custom chip runs. That is
 * the whole point -- `pio test -e native` (or a plain g++ build) exercises the
 * identical behaviour a student will see light up on the canvas.
 *
 * Construct it with the module lineup you want in the virtual base:
 *     const char *lineup[] = { "P1-08TRS" };
 *     MockTransport bus(lineup, 1);
 *     P1AM_Sim P1(bus);
 */
#ifndef P1AM_MOCK_TRANSPORT_H
#define P1AM_MOCK_TRANSPORT_H

#include "P1AM_Sim.h"
#include "base_model.h"

class MockTransport : public IBaseTransport {
public:
    MockTransport(const char *const *lineup, uint8_t count) {
        bm_init(&_model, lineup, count);
    }

    void begin() override { /* nothing to do for the in-process model */ }

    void command(const uint8_t *cmd, uint8_t cmd_len,
                 uint8_t *resp, uint8_t resp_len) override {
        uint8_t scratch[P1_ENUM_RESP_LEN];
        size_t produced = bm_handle(&_model, cmd, cmd_len, scratch);
        for (uint8_t i = 0; i < resp_len; i++) {
            resp[i] = (i < produced) ? scratch[i] : 0;
        }
    }

    /* Test-only introspection: peek at the raw output image for a slot. */
    uint8_t rawOutByte(uint8_t slot, uint8_t byteIndex) const {
        if (slot < 1 || slot > P1_MAX_SLOTS || byteIndex >= P1_MAX_DO_BYTES) return 0;
        return _model.out_image[slot - 1][byteIndex];
    }

    /* Test-only: inject an analog reading (what a real module's ADC would see).
     * On the Wokwi chip the same image is populated from control sliders. */
    void setAnalog(uint8_t slot, uint8_t channel, int32_t counts) {
        bm_set_analog(&_model, slot, channel, (uint32_t)counts);
    }
    void setTemperature(uint8_t slot, uint8_t channel, float celsius) {
        uint32_t bits;
        memcpy(&bits, &celsius, sizeof(bits));
        bm_set_analog(&_model, slot, channel, bits);
    }

private:
    p1_base_model_t _model;
};

#endif /* P1AM_MOCK_TRANSPORT_H */
