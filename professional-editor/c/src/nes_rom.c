/*
 * nes_rom.c -- iNES ROM container + text/menu editing (C implementation).
 * Part of the mearvk/Nintendo Professional Editor. No copyrighted ROM content.
 */
#include "nes_rom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- character table -------------------------------------------------- */

struct nes_table {
    int      loaded;          /* 0 => ASCII identity                        */
    int16_t  glyph_to_byte[256]; /* ASCII glyph -> byte, -1 if unmapped     */
    uint8_t  byte_to_glyph[256]; /* byte -> ASCII glyph (0 if none)         */
    uint8_t  fill_byte;       /* pad byte for short replacements            */
};

nes_table *nes_table_new(void) {
    nes_table *t = (nes_table *)calloc(1, sizeof(*t));
    if (!t) return NULL;
    for (int i = 0; i < 256; i++) t->glyph_to_byte[i] = -1;
    t->fill_byte = 0x00;
    return t;
}

void nes_table_free(nes_table *t) { free(t); }

static int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

nes_status nes_table_load(nes_table *t, const char *path) {
    if (!t || !path) return NES_ERR_ARG;
    FILE *f = fopen(path, "rb");
    if (!f) return NES_ERR_IO;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        int hi = hexval((unsigned char)line[0]);
        int lo = hexval((unsigned char)line[1]);
        if (hi < 0 || lo < 0 || line[2] != '=') continue;
        unsigned byte = (unsigned)(hi * 16 + lo);
        unsigned char glyph = (unsigned char)line[3];
        if (glyph == '\n' || glyph == '\r' || glyph == '\0') glyph = ' ';
        t->byte_to_glyph[byte] = glyph;
        if (t->glyph_to_byte[glyph] < 0) t->glyph_to_byte[glyph] = (int16_t)byte;
        if (glyph == ' ') t->fill_byte = (uint8_t)byte; /* prefer table space */
    }
    fclose(f);
    t->loaded = 1;
    return NES_OK;
}

/* ---- ROM lifecycle ---------------------------------------------------- */

static void nes_resolve_regions(nes_rom *rom) {
    const uint8_t *h = rom->data;
    rom->prg_banks = h[4];
    rom->chr_banks = h[5];
    rom->has_trainer = (h[6] & 0x04) ? 1 : 0;
    rom->mapper = (unsigned)((h[6] >> 4) | (h[7] & 0xF0));
    rom->prg_offset = NES_HEADER_SIZE + (rom->has_trainer ? NES_TRAINER_SIZE : 0u);
    rom->prg_size = (size_t)rom->prg_banks * NES_PRG_BANK_SIZE;
    rom->chr_size = (size_t)rom->chr_banks * NES_CHR_BANK_SIZE;
    rom->chr_offset = rom->chr_size ? (rom->prg_offset + rom->prg_size) : 0u;
}

nes_status nes_rom_load(const char *path, nes_rom *rom) {
    if (!path || !rom) return NES_ERR_ARG;
    memset(rom, 0, sizeof(*rom));
    FILE *f = fopen(path, "rb");
    if (!f) return NES_ERR_IO;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NES_ERR_IO; }
    long sz = ftell(f);
    if (sz < (long)NES_HEADER_SIZE) { fclose(f); return NES_ERR_FORMAT; }
    rewind(f);
    rom->data = (uint8_t *)malloc((size_t)sz);
    if (!rom->data) { fclose(f); return NES_ERR_IO; }
    if (fread(rom->data, 1, (size_t)sz, f) != (size_t)sz) {
        free(rom->data); rom->data = NULL; fclose(f); return NES_ERR_IO;
    }
    fclose(f);
    rom->size = (size_t)sz;
    if (memcmp(rom->data, "NES\x1A", 4) != 0) {
        free(rom->data); rom->data = NULL; return NES_ERR_FORMAT;
    }
    nes_resolve_regions(rom);
    if (rom->prg_offset + rom->prg_size > rom->size) {
        free(rom->data); rom->data = NULL; return NES_ERR_FORMAT;
    }
    return NES_OK;
}

nes_status nes_rom_save(const nes_rom *rom, const char *path) {
    if (!rom || !rom->data || !path) return NES_ERR_ARG;
    FILE *f = fopen(path, "wb");
    if (!f) return NES_ERR_IO;
    size_t w = fwrite(rom->data, 1, rom->size, f);
    fclose(f);
    return (w == rom->size) ? NES_OK : NES_ERR_IO;
}

void nes_rom_free(nes_rom *rom) {
    if (rom && rom->data) { free(rom->data); rom->data = NULL; rom->size = 0; }
}

/* ---- text / menu editing --------------------------------------------- */

static int slot_in_rom(const nes_rom *rom, size_t offset, size_t len) {
    return offset <= rom->size && len <= rom->size && offset + len <= rom->size;
}

nes_status nes_decode(const nes_rom *rom, const nes_table *tbl,
                      size_t offset, size_t len, char *out) {
    if (!rom || !out) return NES_ERR_ARG;
    if (!slot_in_rom(rom, offset, len)) return NES_ERR_RANGE;
    for (size_t i = 0; i < len; i++) {
        uint8_t b = rom->data[offset + i];
        char g;
        if (!tbl || !tbl->loaded) {
            g = (b >= 0x20 && b < 0x7F) ? (char)b : '.';
        } else {
            uint8_t m = tbl->byte_to_glyph[b];
            g = m ? (char)m : '.';
        }
        out[i] = g;
    }
    out[len] = '\0';
    return NES_OK;
}

/* Encode `text` into `buf` (len bytes), padding with the fill byte. */
static nes_status encode_into(const nes_table *tbl, const char *text,
                              size_t len, uint8_t *buf) {
    size_t tl = strlen(text);
    if (tl > len) return NES_ERR_TOO_LONG;
    uint8_t fill = (tbl && tbl->loaded) ? tbl->fill_byte : (uint8_t)0x00;
    for (size_t i = 0; i < len; i++) {
        if (i < tl) {
            unsigned char g = (unsigned char)text[i];
            if (!tbl || !tbl->loaded) {
                buf[i] = (uint8_t)g;
            } else {
                int16_t b = tbl->glyph_to_byte[g];
                if (b < 0) return NES_ERR_ENCODE;
                buf[i] = (uint8_t)b;
            }
        } else {
            buf[i] = fill;
        }
    }
    return NES_OK;
}

nes_status nes_set_text(nes_rom *rom, const nes_table *tbl,
                        size_t offset, size_t len, const char *text) {
    if (!rom || !rom->data || !text) return NES_ERR_ARG;
    if (!slot_in_rom(rom, offset, len)) return NES_ERR_RANGE;
    uint8_t *buf = (uint8_t *)malloc(len ? len : 1);
    if (!buf) return NES_ERR_IO;
    nes_status s = encode_into(tbl, text, len, buf);
    if (s == NES_OK) memcpy(rom->data + offset, buf, len);
    free(buf);
    return s;
}

nes_status nes_expect_text(const nes_rom *rom, const nes_table *tbl,
                           size_t offset, size_t len, const char *expected) {
    if (!rom || !expected) return NES_ERR_ARG;
    if (!slot_in_rom(rom, offset, len)) return NES_ERR_RANGE;
    char *buf = (char *)malloc(len + 1);
    if (!buf) return NES_ERR_IO;
    nes_status s = nes_decode(rom, tbl, offset, len, buf);
    if (s == NES_OK) {
        /* Compare ignoring trailing fill ('.'/space) so a padded slot matches. */
        size_t el = strlen(expected);
        int ok = (el <= len) && (strncmp(buf, expected, el) == 0);
        for (size_t i = el; ok && i < len; i++)
            if (buf[i] != '.' && buf[i] != ' ') ok = 0;
        s = ok ? NES_OK : NES_ERR_MISMATCH;
    }
    free(buf);
    return s;
}

const char *nes_strerror(nes_status s) {
    switch (s) {
        case NES_OK:          return "ok";
        case NES_ERR_IO:      return "I/O error";
        case NES_ERR_FORMAT:  return "not a valid iNES image";
        case NES_ERR_RANGE:   return "offset/length out of range";
        case NES_ERR_TOO_LONG:return "replacement longer than slot";
        case NES_ERR_MISMATCH:return "find guard did not match";
        case NES_ERR_ENCODE:  return "glyph not in character table";
        case NES_ERR_ARG:     return "bad argument";
        default:              return "unknown error";
    }
}
