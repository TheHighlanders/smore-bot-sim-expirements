/*
 * p1-base-controller  --  Wokwi custom chip: the P1AM Base Controller.
 *
 * Acts as an SPI *peripheral* via the Wokwi Chips SPI API (spi_init/spi_start/
 * spi_stop + a `done` callback; chip-select is NOT handled by the API, so we
 * watch the CS pin ourselves). [ref: docs/references/wokwi/chips-api-spi.md]
 * The RP2040 (running the P1AM_Sim SpiTransport) is the SPI controller. We
 * implement the two-phase, ACK-gated protocol from shared/base_model.h and
 * drive OUT1..OUT8 to reflect slot 1's discrete-output image -- so the wired
 * LEDs / relay part visibly turn on.
 *
 * Because the command handling is bm_handle() from the shared header, this
 * chip and the host unit-test MockTransport are guaranteed to behave the same.
 *
 * SPI framing (see base_model.h for the full description):
 *   phase 0: CS low, controller clocks the command frame in, CS high.
 *            -> we run bm_handle(), refresh outputs, raise ACK.
 *   phase 1: CS low, controller clocks the fixed-length response out, CS high.
 *            -> we lower ACK.
 * We always spi_start() with a buffer larger than any frame so `done` fires on
 * CS de-assert (spi_stop), never on a full buffer -- keeping exactly one `done`
 * per chip-select cycle.
 */
#include "wokwi-api.h"
#include "base_model.h"
#include <stdlib.h>
#include <string.h>

#define XFER_BUF 96   /* > P1_ENUM_RESP_LEN (61) so `done` only fires on CS high */

typedef struct {
    spi_dev_t spi;
    pin_t     cs;
    pin_t     ack;
    pin_t     en;
    pin_t     out[8];

    p1_base_model_t model;

    uint8_t buf[XFER_BUF];
    uint8_t staged[P1_ENUM_RESP_LEN];
    uint8_t staged_len;
    int     phase;        /* 0 = expecting command, 1 = sending response */
} chip_state_t;

/* Reflect slot 1's output image onto the OUT pins (LSB = channel 1). */
static void refresh_outputs(chip_state_t *c) {
    uint8_t img = c->model.out_image[0][0];
    for (int i = 0; i < 8; i++) {
        pin_write(c->out[i], ((img >> i) & 0x1) ? HIGH : LOW);
    }
}

static void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count) {
    chip_state_t *c = (chip_state_t *)user_data;
    if (c->phase == 0) {
        /* Received `count` command bytes. Compute + stage the response. */
        c->staged_len = (uint8_t)bm_handle(&c->model, buffer, count, c->staged);
        refresh_outputs(c);
        c->phase = 1;
        pin_write(c->ack, HIGH);   /* response ready */
    } else {
        /* Response has been clocked out. */
        c->phase = 0;
        pin_write(c->ack, LOW);    /* transaction complete */
    }
}

static void chip_cs_change(void *user_data, pin_t pin, uint32_t value) {
    chip_state_t *c = (chip_state_t *)user_data;
    (void)pin;
    if (value == LOW) {
        if (c->phase == 1) {
            /* Preload the staged response so it is clocked out on MISO. */
            memset(c->buf, 0, sizeof(c->buf));
            memcpy(c->buf, c->staged, c->staged_len);
        }
        spi_start(c->spi, c->buf, sizeof(c->buf));
    } else {
        spi_stop(c->spi);   /* triggers chip_spi_done with the byte count */
    }
}

void chip_init(void) {
    chip_state_t *c = (chip_state_t *)malloc(sizeof(chip_state_t));
    memset(c, 0, sizeof(*c));

    /* Configure the simulated base: one P1-08TRS relay module in slot 1.
     * P1-08TRS = 8-pt relay output (6x Form A + 2x Form C), ID 0x1404008F.
     * [ref: docs/references/p1am-library.md#module-ids -> Module_List.h:66;
     *  part specs: docs/references/datasheets/P1-08TRS.md] */
    const char *lineup[] = { "P1-08TRS" };
    bm_init(&c->model, lineup, 1);

    /* Control / handshake pins. */
    c->cs  = pin_init("CS",  INPUT_PULLUP);
    c->ack = pin_init("ACK", OUTPUT);
    pin_write(c->ack, LOW);
    c->en  = pin_init("EN",  INPUT);   /* base-enable; present for fidelity */

    /* Discrete output pins -> LEDs / relay part in the diagram. */
    static const char *out_names[8] = {
        "OUT1", "OUT2", "OUT3", "OUT4", "OUT5", "OUT6", "OUT7", "OUT8"
    };
    for (int i = 0; i < 8; i++) {
        c->out[i] = pin_init(out_names[i], OUTPUT);
        pin_write(c->out[i], LOW);
    }

    /* SPI peripheral. Mode 0 (real hardware uses mode 2 -- see base_model.h). */
    const spi_config_t spi_cfg = {
        .sck       = pin_init("SCK",  INPUT),
        .mosi      = pin_init("MOSI", INPUT),
        .miso      = pin_init("MISO", INPUT),
        .mode      = 0,
        .done      = chip_spi_done,
        .user_data = c,
    };
    c->spi = spi_init(&spi_cfg);

    /* Chip-select drives the phase state machine. */
    const pin_watch_config_t cs_watch = {
        .edge       = BOTH,
        .pin_change = chip_cs_change,
        .user_data  = c,
    };
    pin_watch(c->cs, &cs_watch);

    c->phase = 0;
}
