/*
 * base_model.h  --  Behavioural model of the P1AM "Base Controller".
 *
 * On real hardware the P1AM-100/200 CPU talks over SPI to a Base Controller
 * chip, which fans commands out to the stacked P1000 I/O modules. This header
 * is a *simplified, self-consistent* model of that base controller: it owns
 * the per-slot output image and knows how to answer the three commands this
 * scaffold uses (enumerate / write-discrete / read-discrete).
 *
 * It is plain C and is compiled into TWO places:
 *   1. the Wokwi custom chip (wokwi/chips/p1-base-controller) -- the "hardware"
 *   2. the host MockTransport (lib/P1AM_Sim) -- used by the unit tests
 * Because both sides call the exact same bm_handle(), a test that passes on the
 * host is exercising the same logic the student sees light up in Wokwi.
 *
 * PROTOCOL (simplified for teaching -- see docs/DESIGN.md for the real one):
 *   Every command is a two-phase, chip-select-framed exchange gated by an ACK
 *   line, and every command returns a FIXED-length response so the SPI master
 *   always knows how many bytes to clock back:
 *
 *     Phase 1 (command):  master pulls CS low, clocks the command frame, CS high
 *     Phase 2 (response): master waits for ACK high, pulls CS low, clocks
 *                         `resp_len` dummy bytes to read the response, CS high,
 *                         waits for ACK low.
 *
 * Opcodes match the real library where we implement them
 * [ref: docs/references/p1am-library.md#opcodes -> defines.h:56-77].
 *
 * NOTE ON FIDELITY: the real base controller uses SPI mode 2 @ 1 MHz, MSB-first
 * [ref: p1am-library.md#spi-bus -> P1AM.cpp:27], and a 3-edge ACK pulse
 * (high->low->high) per command [ref: p1am-library.md#ack-handshake ->
 * P1AM.cpp:1296-1331]. We use SPI mode 0 and a simple high/low ACK strobe so
 * the scaffold is robust and easy to reason about. The opcodes, little-endian
 * layout, LSB-first channel packing, and slot/channel = 1-based conventions all
 * match the real hardware [ref: p1am-library.md#discrete-api,
 * #discrete-data-model].  All refs are relative to docs/references/.
 */
#ifndef P1AM_BASE_MODEL_H
#define P1AM_BASE_MODEL_H

#include "module_db.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- protocol constants ----
 * Opcode values quoted from the real library.
 * [ref: docs/references/p1am-library.md#opcodes -> defines.h:56-77] */
#define P1_MOD_HDR              0x02   /* enumerate: how many modules & their IDs */
#define P1_VERSION_HDR          0x03   /* base firmware version */
#define P1_CFG_HDR              0x10   /* configure a module (analog modules) */
#define P1_READ_DISCRETE_HDR    0x50   /* read a module's discrete image */
#define P1_READ_ANALOG_HDR      0x51   /* read one analog input channel (4 bytes) */
#define P1_WRITE_DISCRETE_HDR   0x60   /* write a module's discrete outputs */

/* Fixed response lengths (bytes the master clocks back in phase 2).
 * P1_MAX_SLOTS / P1_MAX_DO_BYTES / P1_MAX_AI_BYTES come from module_db.h. */
#define P1_ENUM_RESP_LEN        (1 + 4 * P1_MAX_SLOTS)  /* count + 4 bytes/slot   = 61 */
#define P1_WRITE_RESP_LEN       1      /* status byte                                   */
#define P1_READ_RESP_LEN        4      /* discrete image as little-endian uint32        */
#define P1_ANALOG_RESP_LEN      4      /* one analog channel: int32 little-endian        */
#define P1_VERSION_RESP_LEN     4      /* firmware version as little-endian uint32       */
#define P1_STATUS_OK            0x00
#define P1_STATUS_BAD_SLOT      0xEE

/* A simulated base controller: per-slot discrete-output and analog-input images. */
typedef struct {
    uint8_t  num_modules;                             /* 0..P1_MAX_SLOTS */
    uint32_t module_id[P1_MAX_SLOTS];                 /* per slot (index 0 == slot 1) */
    uint8_t  do_bytes[P1_MAX_SLOTS];                  /* cached from the module db     */
    uint8_t  di_bytes[P1_MAX_SLOTS];                  /* cached from the module db     */
    uint8_t  ai_bytes[P1_MAX_SLOTS];                  /* cached from the module db     */
    uint8_t  out_image[P1_MAX_SLOTS][P1_MAX_DO_BYTES];/* discrete output (LSB=ch1)     */
    uint8_t  in_image[P1_MAX_SLOTS][P1_MAX_DO_BYTES]; /* discrete input  (LSB=ch1)     */
    uint8_t  ai_image[P1_MAX_SLOTS][P1_MAX_AI_BYTES]; /* analog input: 4 bytes LE/ch   */
    uint32_t fw_version;                              /* reported by VERSION_HDR       */
} p1_base_model_t;

/* Reset all outputs to 0 but keep the configured module lineup. */
static inline void bm_clear_outputs(p1_base_model_t *m) {
    memset(m->out_image, 0, sizeof(m->out_image));
}

/* Configure the simulated base's module lineup from an array of part-number
 * strings (slot 1 first). Unknown names are skipped. */
static inline void bm_init(p1_base_model_t *m, const char *const *names, uint8_t count) {
    memset(m, 0, sizeof(*m));
    m->fw_version = 0x00010000UL; /* v1.0.00 */
    uint8_t slot = 0;
    for (uint8_t i = 0; i < count && slot < P1_MAX_SLOTS; i++) {
        const p1_module_props_t *p = mdb_lookup_name(names[i]);
        if (p->id == P1_MODULE_UNKNOWN.id) {
            continue;
        }
        m->module_id[slot] = p->id;
        m->do_bytes[slot]  = p->doBytes;
        m->di_bytes[slot]  = p->diBytes;
        m->ai_bytes[slot]  = p->aiBytes;
        slot++;
    }
    m->num_modules = slot;
}

/* True if `slot` (1-based) is a valid configured slot. */
static inline int bm_valid_slot(const p1_base_model_t *m, uint8_t slot) {
    return slot >= 1 && slot <= m->num_modules;
}

/* Drive a discrete INPUT point (a field sensor wired to an input module).
 * slot/channel are 1-based; bit LSB = channel 1, matching the real bitmap layout
 * [ref: docs/references/facts-docs/api_reference.md:172-180]. */
static inline void bm_set_discrete_in(p1_base_model_t *m, uint8_t slot,
                                      uint8_t channel, int on) {
    if (!bm_valid_slot(m, slot) || channel < 1) return;
    uint8_t idx = (uint8_t)((channel - 1) / 8);
    uint8_t bit = (uint8_t)((channel - 1) % 8);
    if (idx >= P1_MAX_DO_BYTES) return;
    if (on) m->in_image[slot - 1][idx] |=  (uint8_t)(1u << bit);
    else    m->in_image[slot - 1][idx] &= (uint8_t)~(1u << bit);
}

/* Set an analog-input channel's raw value (int32, e.g. counts or float bits).
 * Used by the Wokwi chip (from control sliders) and by host tests to inject an
 * input reading. slot/channel are 1-based; LE across the 4 channel bytes. */
static inline void bm_set_analog(p1_base_model_t *m, uint8_t slot,
                                 uint8_t channel, uint32_t raw) {
    if (!bm_valid_slot(m, slot) || channel < 1) return;
    uint8_t off = (uint8_t)((channel - 1) * 4);
    if ((uint16_t)off + 4 > P1_MAX_AI_BYTES) return;
    uint8_t *img = m->ai_image[slot - 1];
    img[off + 0] = (uint8_t)(raw & 0xFF);
    img[off + 1] = (uint8_t)((raw >> 8) & 0xFF);
    img[off + 2] = (uint8_t)((raw >> 16) & 0xFF);
    img[off + 3] = (uint8_t)((raw >> 24) & 0xFF);
}

/*
 * Handle one command frame. Writes the fixed-length response into `resp`
 * (which must be at least P1_ENUM_RESP_LEN bytes) and returns the response
 * length. An unknown / malformed command yields a single status byte.
 *
 * This is the ONE function both the Wokwi chip and the host mock call.
 */
static inline size_t bm_handle(p1_base_model_t *m,
                               const uint8_t *cmd, size_t cmd_len,
                               uint8_t *resp) {
    if (cmd_len == 0) {
        resp[0] = P1_STATUS_OK;
        return 1;
    }

    switch (cmd[0]) {
    case P1_MOD_HDR: {
        /* Response: [count][id0 LE x4][id1 LE x4]... padded to P1_MAX_SLOTS. */
        memset(resp, 0, P1_ENUM_RESP_LEN);
        resp[0] = m->num_modules;
        for (uint8_t s = 0; s < m->num_modules; s++) {
            uint32_t id = m->module_id[s];
            resp[1 + s * 4 + 0] = (uint8_t)(id & 0xFF);
            resp[1 + s * 4 + 1] = (uint8_t)((id >> 8) & 0xFF);
            resp[1 + s * 4 + 2] = (uint8_t)((id >> 16) & 0xFF);
            resp[1 + s * 4 + 3] = (uint8_t)((id >> 24) & 0xFF);
        }
        return P1_ENUM_RESP_LEN;
    }

    case P1_VERSION_HDR: {
        uint32_t v = m->fw_version;
        resp[0] = (uint8_t)(v & 0xFF);
        resp[1] = (uint8_t)((v >> 8) & 0xFF);
        resp[2] = (uint8_t)((v >> 16) & 0xFF);
        resp[3] = (uint8_t)((v >> 24) & 0xFF);
        return P1_VERSION_RESP_LEN;
    }

    case P1_WRITE_DISCRETE_HDR: {
        /* cmd = [0x60, slot, channel, data...]
         *   channel == 0 : data is `doBytes` little-endian bytes -> whole module
         *   channel != 0 : data[0] bit0 sets/clears that single channel        */
        resp[0] = P1_STATUS_OK;
        if (cmd_len < 3 || !bm_valid_slot(m, cmd[1])) {
            resp[0] = P1_STATUS_BAD_SLOT;
            return P1_WRITE_RESP_LEN;
        }
        uint8_t slot = cmd[1];
        uint8_t ch   = cmd[2];
        uint8_t db   = m->do_bytes[slot - 1];
        uint8_t *img = m->out_image[slot - 1];
        if (ch == 0) {
            for (uint8_t b = 0; b < db && (size_t)(3 + b) < cmd_len; b++) {
                img[b] = cmd[3 + b];
            }
        } else {
            uint8_t bit  = (uint8_t)(ch - 1);
            uint8_t byte = (uint8_t)(bit / 8);
            uint8_t mask = (uint8_t)(1u << (bit % 8));
            if (byte < db && cmd_len >= 4) {
                if (cmd[3] & 0x01) img[byte] |= mask;
                else               img[byte] &= (uint8_t)~mask;
            }
        }
        return P1_WRITE_RESP_LEN;
    }

    case P1_READ_DISCRETE_HDR: {
        /* cmd = [0x50, slot]; response = 4-byte LE discrete image. */
        memset(resp, 0, P1_READ_RESP_LEN);
        if (cmd_len >= 2 && bm_valid_slot(m, cmd[1])) {
            uint8_t slot = cmd[1];
            /* A module with discrete inputs reports those; an output-only module
             * reports a readback of what was written to it. */
            uint8_t db   = m->di_bytes[slot - 1] ? m->di_bytes[slot - 1] : m->do_bytes[slot - 1];
            const uint8_t *img = m->di_bytes[slot - 1] ? m->in_image[slot - 1]
                                                      : m->out_image[slot - 1];
            for (uint8_t b = 0; b < db && b < P1_READ_RESP_LEN; b++) {
                resp[b] = img[b];
            }
        }
        return P1_READ_RESP_LEN;
    }

    case P1_READ_ANALOG_HDR: {
        /* cmd = [0x51, slot, channel]; response = that channel's 4 LE bytes.
         * The raw value is whatever bm_set_analog stored -- int counts for
         * P1-04AD, or the bit pattern of a float for P1-04THM/NTC. */
        memset(resp, 0, P1_ANALOG_RESP_LEN);
        if (cmd_len >= 3 && bm_valid_slot(m, cmd[1]) && cmd[2] >= 1) {
            uint8_t slot = cmd[1];
            uint8_t off  = (uint8_t)((cmd[2] - 1) * 4);
            if ((uint16_t)off + 4 <= P1_MAX_AI_BYTES &&
                off + 4 <= m->ai_bytes[slot - 1]) {
                const uint8_t *img = m->ai_image[slot - 1];
                for (uint8_t b = 0; b < P1_ANALOG_RESP_LEN; b++) {
                    resp[b] = img[off + b];
                }
            }
        }
        return P1_ANALOG_RESP_LEN;
    }

    case P1_CFG_HDR:
        /* cmd = [0x10, slot, config...]. Analog modules carry config bytes; the
         * real base auto-configures them at init. The sim accepts and ACKs the
         * config (values don't change simulated readings), so firmware that
         * configures a module doesn't stall. */
        resp[0] = bm_valid_slot(m, (cmd_len >= 2 ? cmd[1] : 0))
                      ? P1_STATUS_OK : P1_STATUS_BAD_SLOT;
        return P1_WRITE_RESP_LEN;

    default:
        resp[0] = P1_STATUS_OK;
        return 1;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* P1AM_BASE_MODEL_H */
