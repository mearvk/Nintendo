/* ============================================================================
 * Nintendo Technical Series (NTS) — original ROM canvas generator
 * ----------------------------------------------------------------------------
 * Emits a BRAND-NEW, EMPTY NES 2.0 image: a valid 16-byte header declaring a
 * chosen PRG/CHR footprint, optionally followed by a zeroed body you own and
 * fill with your own homebrew code. It copies NOTHING from any existing game.
 *
 * IMPORTANT — this is a FORMAT-CAPACITY tool, not a runnable game:
 *   The NES 2.0 header can *encode* enormous sizes (exponent notation reaches
 *   the gigabyte range on paper). Real mappers, flash carts, and emulators cap
 *   out far lower (practical homebrew is low-single-digit MB). A 200 MB canvas
 *   is a theoretical/format demonstration — it will not boot on hardware or in
 *   an emulator. The generator makes the declared capacity explicit and valid;
 *   it does not pretend the result is playable.
 *
 * NES 2.0 exponent size encoding (per byte 4/9 for PRG, 5/9 for CHR):
 *   when the 12-bit size field's high nibble == 0x0F, the low byte is read as
 *   exponent notation:  size = 2^E * (M*2 + 1), where
 *     E = bits 2..7 of the low byte (6-bit exponent),
 *     M = bits 0..1 of the low byte (2-bit multiplier).
 *   This reaches 2^63 * 7 bytes in principle.
 * ========================================================================== */
#ifndef NTS_CANVAS_H
#define NTS_CANVAS_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    NTS_CANVAS_OK = 0,
    NTS_CANVAS_ERR_RANGE,   /* requested size not representable        */
    NTS_CANVAS_ERR_IO,
    NTS_CANVAS_ERR_NULL
} nts_canvas_status;

typedef struct {
    uint64_t prg_bytes;     /* desired PRG-ROM footprint (bytes)       */
    uint64_t chr_bytes;     /* desired CHR-ROM footprint (bytes, 0 ok) */
    uint16_t mapper;        /* mapper number to declare                */
    uint8_t  submapper;     /* NES 2.0 submapper                       */
    int      battery;       /* declare battery-backed WRAM             */
} nts_canvas_spec;

/* Encode `want_bytes` into a NES 2.0 12-bit size field using exponent
 * notation. Writes the chosen low byte into *lo_byte and the high nibble
 * (always 0x0F for exponent form) into *hi_nibble. `actual_bytes` receives the
 * exact size the encoding represents (>= want_bytes, the smallest encodable
 * value that is >= want_bytes). Returns status. */
nts_canvas_status nts_canvas_encode_exp(uint64_t want_bytes,
                                        uint8_t *lo_byte,
                                        uint8_t *hi_nibble,
                                        uint64_t *actual_bytes);

/* Build the 16-byte NES 2.0 header for a spec into header[16].
 * `actual_prg`/`actual_chr` receive the exact encoded footprints. */
nts_canvas_status nts_canvas_header(const nts_canvas_spec *spec,
                                    uint8_t header[16],
                                    uint64_t *actual_prg,
                                    uint64_t *actual_chr);

/* Write a canvas file. If `body` is non-zero, the full zeroed PRG+CHR body is
 * written after the header (large!); if zero, only the 16-byte header is
 * written (a "header-only" capacity declaration). */
nts_canvas_status nts_canvas_write(const char *path,
                                   const nts_canvas_spec *spec,
                                   int body,
                                   uint64_t *actual_prg,
                                   uint64_t *actual_chr);

const char *nts_canvas_status_str(nts_canvas_status s);

#endif /* NTS_CANVAS_H */
