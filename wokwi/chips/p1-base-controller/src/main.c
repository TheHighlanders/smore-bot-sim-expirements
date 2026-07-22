/*
 * p1-base-controller  --  Wokwi custom chip: the P1AM Base Controller.
 *
 * Acts as an SPI *peripheral* via the Wokwi Chips SPI API (spi_init/spi_start/
 * spi_stop + a `done` callback; chip-select is NOT handled by the API, so we
 * watch the CS pin ourselves). [ref: docs/references/wokwi/chips-api-spi.md]
 * The RP2040 (running the P1AM_Sim SpiTransport) is the SPI controller. We
 * implement the two-phase, ACK-gated protocol from shared/base_model.h.
 *
 * Modelled base lineup (one of each I/O module type in the plan):
 *   slot 1  P1-15TD1   15-ch digital output  -> pins S1_01..S1_15 (LEDs)
 *   slot 2  P1-08TRS    8-ch relay output    -> pins S2_1..S2_8   (relay/LEDs)
 *   slot 3  P1-04AD-2    4-ch analog input    -> control sliders ai_ch1..4
 *   slot 4  P1-04THM     4-ch thermocouple    -> control sliders temp_ch1..4 (deg C)
 * Change the lineup in one place: the bm_init() call in chip_init().
 * [ref: docs/references/p1am-library.md#module-ids for the IDs/byte counts]
 *
 * Digital OUTPUTS drive pins (wire to LEDs / a relay part). Analog/temperature
 * INPUTS have no real source in Wokwi, so their values come from chip control
 * sliders [ref: docs/references/wokwi/chips-api-chip-json.md] and are refreshed
 * into the model on every command.
 *
 * SPI framing: phase 0 (CS low) clocks the command in; on CS high we run
 * bm_handle(), refresh outputs, raise ACK. Phase 1 (CS low) clocks the
 * fixed-length response out; on CS high we lower ACK. XFER_BUF > any frame so
 * `done` fires only on CS de-assert -> exactly one `done` per CS cycle.
 */
#include "wokwi-api.h"
#include "base_model.h"
#include <stdlib.h>
#include <string.h>

#define XFER_BUF     96   /* > P1_ENUM_RESP_LEN (61) */
#define SLOT_DO1     1    /* P1-15TD1 */
#define SLOT_RELAY   2    /* P1-08TRS */
#define SLOT_AIN     3    /* P1-04AD-2 */
#define SLOT_TEMP    4    /* P1-04THM  */

typedef struct {
    spi_dev_t spi;
    pin_t     cs, ack, en;
    pin_t     s1[15];        /* slot 1: P1-15TD1 outputs */
    pin_t     s2[8];         /* slot 2: P1-08TRS outputs */
    uint32_t  ai_attr[4];    /* slot 3: analog-input sliders (counts) */
    uint32_t  temp_attr[4];  /* slot 4: temperature sliders (deg C)   */

    p1_base_model_t model;

    uint8_t buf[XFER_BUF];
    uint8_t staged[P1_ENUM_RESP_LEN];
    uint8_t staged_len;
    int     phase;           /* 0 = expecting command, 1 = sending response */
} chip_state_t;

/* Reflect discrete-output images onto the output pins (LSB = channel 1). */
static void refresh_outputs(chip_state_t *c) {
    uint8_t b0 = c->model.out_image[SLOT_DO1 - 1][0];   /* ch1-8  */
    uint8_t b1 = c->model.out_image[SLOT_DO1 - 1][1];   /* ch9-15 */
    for (int i = 0; i < 8; i++)  pin_write(c->s1[i],     ((b0 >> i) & 1) ? HIGH : LOW);
    for (int i = 0; i < 7; i++)  pin_write(c->s1[8 + i], ((b1 >> i) & 1) ? HIGH : LOW);
    uint8_t r = c->model.out_image[SLOT_RELAY - 1][0];  /* relay ch1-8 */
    for (int i = 0; i < 8; i++)  pin_write(c->s2[i],     ((r >> i) & 1) ? HIGH : LOW);
}

/* Pull analog/temperature readings from the control sliders into the model, so
 * READ_ANALOG serves the current slider values. */
static void refresh_inputs(chip_state_t *c) {
    for (uint8_t ch = 1; ch <= 4; ch++) {
        bm_set_analog(&c->model, SLOT_AIN, ch, attr_read(c->ai_attr[ch - 1]));
        int32_t celsius = (int32_t)attr_read(c->temp_attr[ch - 1]);
        float t = (float)celsius;
        uint32_t bits;
        memcpy(&bits, &t, sizeof(bits));
        bm_set_analog(&c->model, SLOT_TEMP, ch, bits);
    }
}

static void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count) {
    chip_state_t *c = (chip_state_t *)user_data;
    if (c->phase == 0) {
        refresh_inputs(c);                     /* sliders -> analog image */
        c->staged_len = (uint8_t)bm_handle(&c->model, buffer, count, c->staged);
        refresh_outputs(c);                    /* output image -> pins    */
        c->phase = 1;
        pin_write(c->ack, HIGH);               /* response ready          */
    } else {
        c->phase = 0;
        pin_write(c->ack, LOW);                /* transaction complete    */
    }
}

static void chip_cs_change(void *user_data, pin_t pin, uint32_t value) {
    chip_state_t *c = (chip_state_t *)user_data;
    (void)pin;
    if (value == LOW) {
        if (c->phase == 1) {
            memset(c->buf, 0, sizeof(c->buf));
            memcpy(c->buf, c->staged, c->staged_len);
        }
        spi_start(c->spi, c->buf, sizeof(c->buf));
    } else {
        spi_stop(c->spi);
    }
}

void chip_init(void) {
    chip_state_t *c = (chip_state_t *)malloc(sizeof(chip_state_t));
    memset(c, 0, sizeof(*c));

    /* One of each I/O module type. IDs/byte counts from Module_List.h. */
    const char *lineup[] = { "P1-15TD1", "P1-08TRS", "P1-04AD-2", "P1-04THM" };
    bm_init(&c->model, lineup, 4);

    c->cs  = pin_init("CS",  INPUT_PULLUP);
    c->ack = pin_init("ACK", OUTPUT);
    pin_write(c->ack, LOW);
    c->en  = pin_init("EN",  INPUT);

    static const char *s1_names[15] = {
        "S1_01","S1_02","S1_03","S1_04","S1_05","S1_06","S1_07","S1_08",
        "S1_09","S1_10","S1_11","S1_12","S1_13","S1_14","S1_15"
    };
    for (int i = 0; i < 15; i++) { c->s1[i] = pin_init(s1_names[i], OUTPUT); pin_write(c->s1[i], LOW); }
    static const char *s2_names[8] = { "S2_1","S2_2","S2_3","S2_4","S2_5","S2_6","S2_7","S2_8" };
    for (int i = 0; i < 8; i++)  { c->s2[i] = pin_init(s2_names[i], OUTPUT); pin_write(c->s2[i], LOW); }

    /* Analog / temperature input sliders (see chip.json "controls"). */
    static const char *ai_names[4]   = { "ai_ch1","ai_ch2","ai_ch3","ai_ch4" };
    static const char *temp_names[4] = { "temp_ch1","temp_ch2","temp_ch3","temp_ch4" };
    for (int i = 0; i < 4; i++) {
        c->ai_attr[i]   = attr_init(ai_names[i], 0);
        c->temp_attr[i] = attr_init(temp_names[i], 20);   /* default 20 C */
    }

    const spi_config_t spi_cfg = {
        .sck       = pin_init("SCK",  INPUT),
        .mosi      = pin_init("MOSI", INPUT),
        .miso      = pin_init("MISO", INPUT),
        .mode      = 0,                        /* real HW = mode 2; see base_model.h */
        .done      = chip_spi_done,
        .user_data = c,
    };
    c->spi = spi_init(&spi_cfg);

    const pin_watch_config_t cs_watch = {
        .edge       = BOTH,
        .pin_change = chip_cs_change,
        .user_data  = c,
    };
    pin_watch(c->cs, &cs_watch);

    c->phase = 0;
}
