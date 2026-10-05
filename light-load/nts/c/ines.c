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
                     uint32_t file_size, nts_rom *out) {
    uint8_t f6, f7;
    uint32_t cursor;

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
        out->prg_banks |= ((uint32_t)(hi & 0x0F)) << 8;
        out->chr_banks |= ((uint32_t)(hi >> 4))   << 8;
        out->mapper = (uint16_t)((f6 >> 4) | (f7 & 0xF0)
                      | (((uint16_t)(buf[8] & 0x0F)) << 8));
        out->submapper = (uint8_t)(buf[8] >> 4);
    } else {
        out->mapper = (uint16_t)((f6 >> 4) | (f7 & 0xF0));
        out->submapper = 0;
    }

    out->has_battery = (f6 & 0x02) ? 1 : 0;
    out->has_trainer = (f6 & 0x04) ? 1 : 0;
    if (f6 & 0x08)      out->mirroring = NTS_MIRROR_FOUR_SCREEN;
    else if (f6 & 0x01) out->mirroring = NTS_MIRROR_VERTICAL;
    else                out->mirroring = NTS_MIRROR_HORIZONTAL;

    out->uses_chr_ram = (out->chr_banks == 0) ? 1 : 0;
    out->prg_bytes = out->prg_banks * NTS_PRG_BANK_SIZE;
    out->chr_bytes = out->chr_banks * NTS_CHR_BANK_SIZE;

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
        "prg", cursor, out->prg_bytes };
    cursor += out->prg_bytes;

    if (out->chr_bytes) {
        out->regions[out->region_count++] = (nts_region){
            "chr", cursor, out->chr_bytes };
        cursor += out->chr_bytes;
    }

    out->file_size     = file_size;
    out->computed_size = cursor;

    if (file_size && cursor > file_size) return NTS_ERR_SIZE_MISMATCH;
    return NTS_OK;
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

    return nts_parse(header, sizeof(header), (uint32_t)sz, out);
}
