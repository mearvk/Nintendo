/* ============================================================================
 * NTS original ROM canvas generator (implementation).
 * Emits a new, empty NES 2.0 image. Copies nothing from any existing game.
 * ========================================================================== */
#include "canvas.h"
#include <stdio.h>
#include <string.h>

const char *nts_canvas_status_str(nts_canvas_status s) {
    switch (s) {
        case NTS_CANVAS_OK:        return "ok";
        case NTS_CANVAS_ERR_RANGE: return "size not representable in NES 2.0";
        case NTS_CANVAS_ERR_IO:    return "I/O error";
        case NTS_CANVAS_ERR_NULL:  return "null argument";
        default:                   return "unknown";
    }
}

nts_canvas_status nts_canvas_encode_exp(uint64_t want_bytes,
                                        uint8_t *lo_byte,
                                        uint8_t *hi_nibble,
                                        uint64_t *actual_bytes) {
    /* Search multiplier M in {0,1,2,3} and exponent E in {0..63} for the
     * smallest size = 2^E * (M*2+1) that is >= want_bytes. */
    uint64_t best = 0;
    int best_e = -1, best_m = -1;
    int m, e;

    if (!lo_byte || !hi_nibble) return NTS_CANVAS_ERR_NULL;
    if (want_bytes == 0) {
        *lo_byte = 0; *hi_nibble = 0;
        if (actual_bytes) *actual_bytes = 0;
        return NTS_CANVAS_OK;      /* zero size => plain field, not exponent */
    }

    for (m = 0; m < 4; ++m) {
        uint64_t mult = (uint64_t)(m * 2 + 1);
        for (e = 0; e < 64; ++e) {
            uint64_t size;
            /* guard against overflow of 2^e * mult */
            if (e >= 63 && mult > 1) break;
            size = ((uint64_t)1u << e) * mult;
            if (size < want_bytes) continue;
            if (best_e < 0 || size < best) {
                best = size; best_e = e; best_m = m;
            }
        }
    }
    if (best_e < 0) return NTS_CANVAS_ERR_RANGE;

    /* low byte: bits 7..2 = exponent, bits 1..0 = multiplier */
    *lo_byte   = (uint8_t)(((best_e & 0x3F) << 2) | (best_m & 0x03));
    *hi_nibble = 0x0F;             /* signals exponent notation */
    if (actual_bytes) *actual_bytes = best;
    return NTS_CANVAS_OK;
}

nts_canvas_status nts_canvas_header(const nts_canvas_spec *spec,
                                    uint8_t header[16],
                                    uint64_t *actual_prg,
                                    uint64_t *actual_chr) {
    uint8_t prg_lo, prg_hi, chr_lo, chr_hi;
    uint64_t ap = 0, ac = 0;
    nts_canvas_status st;

    if (!spec || !header) return NTS_CANVAS_ERR_NULL;

    st = nts_canvas_encode_exp(spec->prg_bytes, &prg_lo, &prg_hi, &ap);
    if (st != NTS_CANVAS_OK) return st;
    st = nts_canvas_encode_exp(spec->chr_bytes, &chr_lo, &chr_hi, &ac);
    if (st != NTS_CANVAS_OK) return st;

    memset(header, 0, 16);
    header[0] = 'N'; header[1] = 'E'; header[2] = 'S'; header[3] = 0x1A;
    header[4] = prg_lo;             /* PRG size low byte          */
    header[5] = chr_lo;             /* CHR size low byte          */

    /* flags6: mapper low nibble (bits 4-7), battery (bit 1).      */
    header[6] = (uint8_t)(((spec->mapper & 0x0F) << 4)
                          | (spec->battery ? 0x02 : 0x00));
    /* flags7: NES 2.0 id (bits 2-3 = 0b10), mapper mid nibble.    */
    header[7] = (uint8_t)(0x08 | (spec->mapper & 0xF0));
    /* byte 8: mapper high nibble (bits 0-3), submapper (bits 4-7).*/
    header[8] = (uint8_t)(((spec->mapper >> 8) & 0x0F)
                          | ((spec->submapper & 0x0F) << 4));
    /* byte 9: PRG size high nibble (0-3), CHR size high nibble (4-7). */
    header[9] = (uint8_t)((prg_hi & 0x0F) | ((chr_hi & 0x0F) << 4));
    /* bytes 10-15 left zero (no PRG/CHR-RAM declared, NTSC, etc.) */

    if (actual_prg) *actual_prg = ap;
    if (actual_chr) *actual_chr = ac;
    return NTS_CANVAS_OK;
}

nts_canvas_status nts_canvas_write(const char *path,
                                   const nts_canvas_spec *spec,
                                   int body,
                                   uint64_t *actual_prg,
                                   uint64_t *actual_chr) {
    uint8_t header[16];
    uint64_t ap = 0, ac = 0;
    nts_canvas_status st;
    FILE *fp;

    if (!path || !spec) return NTS_CANVAS_ERR_NULL;
    st = nts_canvas_header(spec, header, &ap, &ac);
    if (st != NTS_CANVAS_OK) return st;

    fp = fopen(path, "wb");
    if (!fp) return NTS_CANVAS_ERR_IO;
    if (fwrite(header, 1, 16, fp) != 16) { fclose(fp); return NTS_CANVAS_ERR_IO; }

    if (body) {
        /* stream zeros for PRG + CHR without allocating the whole thing */
        static const uint8_t zeros[65536];
        uint64_t remaining = ap + ac;
        while (remaining) {
            size_t chunk = remaining > sizeof(zeros)
                           ? sizeof(zeros) : (size_t)remaining;
            if (fwrite(zeros, 1, chunk, fp) != chunk) {
                fclose(fp); return NTS_CANVAS_ERR_IO;
            }
            remaining -= chunk;
        }
    }
    fclose(fp);
    if (actual_prg) *actual_prg = ap;
    if (actual_chr) *actual_chr = ac;
    return NTS_CANVAS_OK;
}
