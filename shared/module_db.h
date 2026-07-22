/*
 * module_db.h  --  Productivity1000 (P1-xxx) module property table.
 *
 * This is the single source of truth for "what does each module look like on
 * the wire": its 32-bit ID (as reported by the base controller during
 * enumeration) and how many bytes of discrete-in / discrete-out data it owns.
 *
 * It is plain C (valid C++ too) and is #included by BOTH sides of the sim:
 *   - the master side (the P1AM_Sim shim running on the RP2040), and
 *   - the base side  (base_model.h, compiled into the Wokwi custom chip AND
 *     into the host MockTransport used by the unit tests).
 * Sharing one table is what guarantees the tests and the simulated hardware
 * agree byte-for-byte.
 *
 * The IDs and byte counts below are taken verbatim from the real FACTS
 * Engineering P1AM library, src/Module_List.h. We only model discrete modules
 * in this scaffold; analog/specialty modules can be added by extending the table.
 *
 * CITATION: docs/references/p1am-library.md#module-ids
 *   (local snapshot: docs/references/p1am-library/Module_List.h;
 *    upstream: github.com/facts-engineering/P1AM v1.0.10, src/Module_List.h)
 */
#ifndef P1AM_MODULE_DB_H
#define P1AM_MODULE_DB_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Physical limits of the (simulated) base. */
#define P1_MAX_SLOTS     15   /* max modules in a base                       */
#define P1_MAX_DO_BYTES  4    /* max discrete-output bytes/module (32 chans) */
#define P1_MAX_AI_BYTES  16   /* max analog-input bytes/module (4 ch x 4 B)  */

/* Per-module properties. Mirrors the real library's `moduleProps` struct
 * (Module_List.h:28-38), trimmed to the fields this scaffold uses. Analog uses
 * 4 bytes (int32, little-endian) per channel, so aiBytes = 4 * analog channels. */
typedef struct {
    uint32_t    id;          /* 32-bit module ID reported during enumeration */
    uint8_t     diBytes;     /* # bytes of discrete input  (channels/8, rounded up) */
    uint8_t     doBytes;     /* # bytes of discrete output */
    uint8_t     aiBytes;     /* # bytes of analog input (4 * analog channels) */
    uint8_t     configBytes; /* # config bytes (>0 => module needs configuration) */
    uint8_t     channels;    /* # I/O channels (for convenience / labelling) */
    const char *name;        /* human-readable part number */
} p1_module_props_t;

/* The P1000 catalog this scaffold knows about. Every id / byte count is quoted
 * from Module_List.h at the cited line; channels/name are for readability.
 * IDs verified against docs/references/p1am-library/Module_List.h (v1.0.10).
 * Output-type/channel descriptions: docs/references/facts-docs/<part>.md. */
static const p1_module_props_t P1_MODULE_DB[] = {
    /*  id           di do ai cfg ch  name          Module_List.h line + kind */
    /* --- discrete output --- */
    { 0x1404008FUL,  0, 1,  0,  0,  8, "P1-08TRS"  }, /* :66  8-pt relay (6x Form A + 2x Form C) */
    { 0x14040091UL,  0, 2,  0,  0, 16, "P1-16TR"   }, /* :68  16-pt relay output */
    { 0x14050081UL,  0, 1,  0,  0,  8, "P1-08TD1"  }, /* :72  8-pt sinking DC output */
    { 0x14050082UL,  0, 1,  0,  0,  8, "P1-08TD2"  }, /* :74  8-pt sourcing DC output */
    { 0x14080085UL,  0, 2,  0,  0, 15, "P1-15TD1"  }, /* :76  15-pt sinking DC output */
    { 0x14080086UL,  0, 2,  0,  0, 15, "P1-15TD2"  }, /* :78  15-pt sourcing DC output */
    /* --- discrete input / combo --- */
    { 0x04A00081UL,  1, 0,  0,  0,  8, "P1-08ND3"  }, /* :46  8-pt DC input */
    { 0x24A50081UL,  1, 1,  0,  0,  8, "P1-16CDR"  }, /* :80  8 DC in + 8 relay out combo */
    /* --- analog input (4 ch x 4 bytes => aiBytes 16) --- */
    { 0x34605583UL,  0, 0, 16,  2,  4, "P1-04AD-2" }, /* :90  4-ch analog voltage/current in */
    { 0x34605590UL,  0, 0, 16,  2,  4, "P1-04ADL-2"}, /* :96  4-ch low-cost analog in */
    /* --- temperature (read as analog; readTemperature reinterprets as float) --- */
    { 0x34608C81UL,  0, 0, 16, 20,  4, "P1-04THM"  }, /* :98  4-ch thermocouple */
    { 0x34608C8EUL,  0, 0, 16,  8,  4, "P1-04NTC"  }, /* :102 4-ch thermistor (NTC) */
};

#define P1_MODULE_DB_COUNT (sizeof(P1_MODULE_DB) / sizeof(P1_MODULE_DB[0]))

/* Sentinel returned for an ID we don't recognise. */
static const p1_module_props_t P1_MODULE_UNKNOWN =
    { 0xFFFFFFFFUL, 0, 0, 0, 0, 0, "UNKNOWN" };

/* Look up a module by its 32-bit ID. Never returns NULL; returns a pointer to
 * P1_MODULE_UNKNOWN for an unrecognised ID. */
static inline const p1_module_props_t *mdb_lookup(uint32_t id) {
    for (size_t i = 0; i < P1_MODULE_DB_COUNT; i++) {
        if (P1_MODULE_DB[i].id == id) {
            return &P1_MODULE_DB[i];
        }
    }
    return &P1_MODULE_UNKNOWN;
}

/* Look up a module by part-number string (used when configuring the sim). */
static inline const p1_module_props_t *mdb_lookup_name(const char *name) {
    for (size_t i = 0; i < P1_MODULE_DB_COUNT; i++) {
        const char *a = P1_MODULE_DB[i].name;
        const char *b = name;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == 0 && *b == 0) {
            return &P1_MODULE_DB[i];
        }
    }
    return &P1_MODULE_UNKNOWN;
}

#ifdef __cplusplus
}
#endif

#endif /* P1AM_MODULE_DB_H */
