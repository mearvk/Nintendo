/* ============================================================================
 * Nintendo Technical Series (NTS) — iNES structural reader (implementation)
 * Reads header + region layout only. No game content is interpreted.
 * ========================================================================== */
#include "ines.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *nts_status_str(nts_status s) {
    switch (s) {
        case NTS_OK:               return "ok";
        case NTS_ERR_IO:           return "I/O error";
        case NTS_ERR_TOO_SMALL:    return "file smaller than iNES header";
        case NTS_ERR_BAD_MAGIC:    return "missing NES\\x1A magic";
        case NTS_ERR_SIZE_MISMATCH:return "declared size exceeds file size";
        case NTS_ERR_NULL:         return "null argument";
        default:                   return "unknown";
    }
}

const char *nts_mirroring_str(nts_mirroring m) {
    switch (m) {
        case NTS_MIRROR_HORIZONTAL:  return "horizontal";
        case NTS_MIRROR_VERTICAL:    return "vertical";
        case NTS_MIRROR_FOUR_SCREEN: return "four-screen";
        default:                     return "unknown";
    }
}

uint32_t nts_region_sum(const uint8_t *data, uint32_t off, uint32_t len) {
    /* Rotate-and-add additive fingerprint; stable, order-sensitive. */
    uint32_t h = 2166136261u; /* FNV-ish seed, used as a plain accumulator */
    uint32_t i;
    if (!data) return 0u;
    for (i = 0; i < len; ++i) {
        h = (h ^ data[off + i]) * 16777619u;
    }
    return h;
}

nts_status nts_parse(const uint8_t *buf, size_t buf_len,
                     uint64_t file_size, nts_rom *out) {
    uint8_t f6, f7;
    uint64_t cursor;

    if (!buf || !out) return NTS_ERR_NULL;
    if (buf_len < NTS_INES_HEADER_SIZE) return NTS_ERR_TOO_SMALL;

    if (!(buf[0] == 'N' && buf[1] == 'E' && buf[2] == 'S' && buf[3] == 0x1A))
        return NTS_ERR_BAD_MAGIC;

    memset(out, 0, sizeof(*out));
    memcpy(out->raw_header, buf, NTS_INES_HEADER_SIZE);

    f6 = buf[6];
    f7 = buf[7];

    /* NES 2.0 detection: flags7 bits 2-3 == 0b10. */
    out->format = ((f7 & 0x0C) == 0x08) ? NTS_FMT_NES20 : NTS_FMT_INES;

    out->prg_banks = buf[4];
    out->chr_banks = buf[5];

    if (out->format == NTS_FMT_NES20) {
        /* High nibbles of PRG/CHR size live in byte 9. */
        uint8_t hi = buf[9];
        uint16_t prg12 = (uint16_t)(buf[4] | (((uint16_t)(hi & 0x0F)) << 8));
        uint16_t chr12 = (uint16_t)(buf[5] | (((uint16_t)(hi >> 4))   << 8));
        out->prg_banks = prg12;
        out->chr_banks = chr12;
        out->mapper = (uint16_t)((f6 >> 4) | (f7 & 0xF0)
                      | (((uint16_t)(buf[8] & 0x0F)) << 8));
        out->submapper = (uint8_t)(buf[8] >> 4);

        /* Large-memory size decode (handles exponent notation). */
        out->prg_bytes = nts_nes20_size(prg12, NTS_PRG_BANK_SIZE,
                                        &out->exponent_prg);
        out->chr_bytes = nts_nes20_size(chr12, NTS_CHR_BANK_SIZE,
                                        &out->exponent_chr);

        /* RAM shift counts: byte 10 (PRG-RAM), byte 11 (CHR-RAM).
         * Low nibble = volatile, high nibble = non-volatile; each is a
         * shift so bytes = 64 << n (0 => none). We sum both halves. */
        {
            uint8_t pr = buf[10], cr = buf[11];
            uint8_t pr_lo = pr & 0x0F, pr_hi = pr >> 4;
            uint8_t cr_lo = cr & 0x0F, cr_hi = cr >> 4;
            out->prg_ram_bytes =
                (pr_lo ? (uint64_t)64u << pr_lo : 0u) +
                (pr_hi ? (uint64_t)64u << pr_hi : 0u);
            out->chr_ram_bytes =
                (cr_lo ? (uint64_t)64u << cr_lo : 0u) +
                (cr_hi ? (uint64_t)64u << cr_hi : 0u);
        }
    } else {
        out->mapper = (uint16_t)((f6 >> 4) | (f7 & 0xF0));
        out->submapper = 0;
        out->prg_bytes = (uint64_t)out->prg_banks * NTS_PRG_BANK_SIZE;
        out->chr_bytes = (uint64_t)out->chr_banks * NTS_CHR_BANK_SIZE;
    }

    out->has_battery = (f6 & 0x02) ? 1 : 0;
    out->has_trainer = (f6 & 0x04) ? 1 : 0;
    if (f6 & 0x08)      out->mirroring = NTS_MIRROR_FOUR_SCREEN;
    else if (f6 & 0x01) out->mirroring = NTS_MIRROR_VERTICAL;
    else                out->mirroring = NTS_MIRROR_HORIZONTAL;

    out->uses_chr_ram = (out->chr_banks == 0) ? 1 : 0;

    /* Build region table. */
    cursor = 0;
    out->region_count = 0;
    out->regions[out->region_count++] = (nts_region){
        "header", 0u, NTS_INES_HEADER_SIZE };
    cursor += NTS_INES_HEADER_SIZE;

    if (out->has_trainer) {
        out->regions[out->region_count++] = (nts_region){
            "trainer", cursor, NTS_TRAINER_SIZE };
        cursor += NTS_TRAINER_SIZE;
    }
    out->regions[out->region_count++] = (nts_region){
        "prg", (uint32_t)cursor, (uint32_t)out->prg_bytes };
    cursor += out->prg_bytes;

    if (out->chr_bytes) {
        out->regions[out->region_count++] = (nts_region){
            "chr", (uint32_t)cursor, (uint32_t)out->chr_bytes };
        cursor += out->chr_bytes;
    }

    out->file_size     = file_size;
    out->computed_size = cursor;

    /* Split the two very different "size disagreement" states:
     *   truncated  : declared layout is LARGER than the file   -> corruption
     *   trailer    : file is LARGER than the declared layout    -> benign footer
     * We no longer treat a trailer as a hard parse error; callers decide. */
    if (file_size && cursor > file_size) {
        out->truncated     = 1;
        out->trailer_bytes = 0;
        return NTS_ERR_SIZE_MISMATCH;
    }
    out->truncated     = 0;
    out->trailer_bytes = file_size ? (int64_t)(file_size - cursor) : 0;
    return NTS_OK;
}

uint64_t nts_nes20_size(uint16_t low12, uint32_t unit, int *is_exponent) {
    if (is_exponent) *is_exponent = 0;
    /* Exponent notation when the high nibble (bits 8-11) is 0x0F. */
    if ((low12 & 0x0F00u) == 0x0F00u) {
        uint8_t mm = (uint8_t)(low12 & 0x03u);          /* multiplier bits */
        uint8_t ex = (uint8_t)((low12 >> 2) & 0x3Fu);    /* exponent bits   */
        if (is_exponent) *is_exponent = 1;
        /* size = 2^exponent * (multiplier*2 + 1) bytes */
        return ((uint64_t)1u << ex) * (uint64_t)(mm * 2u + 1u);
    }
    return (uint64_t)low12 * unit;
}

nts_status nts_read_file(const char *path, nts_rom *out) {
    FILE *fp;
    long sz;
    uint8_t header[NTS_INES_HEADER_SIZE];
    size_t got;

    if (!path || !out) return NTS_ERR_NULL;
    fp = fopen(path, "rb");
    if (!fp) return NTS_ERR_IO;

    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return NTS_ERR_IO; }
    sz = ftell(fp);
    if (sz < 0) { fclose(fp); return NTS_ERR_IO; }
    if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return NTS_ERR_IO; }

    got = fread(header, 1, sizeof(header), fp);
    fclose(fp);
    if (got < sizeof(header)) return NTS_ERR_TOO_SMALL;

    return nts_parse(header, sizeof(header), (uint64_t)sz, out);
}
