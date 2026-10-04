/*
 * nes_rom.h -- iNES ROM container + text/menu editing (C API).
 *
 * Part of the mearvk/Nintendo Professional Editor. This library edits an iNES
 * .nes image the user already owns; it ships no copyrighted ROM content.
 *
 * The API is non-destructive by contract: text replacement is bounded by the
 * original slot length so pointers and bank boundaries are never disturbed.
 */
#ifndef NES_ROM_H
#define NES_ROM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NES_HEADER_SIZE 16u
#define NES_PRG_BANK_SIZE 16384u
#define NES_CHR_BANK_SIZE 8192u
#define NES_TRAINER_SIZE 512u

typedef enum {
    NES_OK = 0,
    NES_ERR_IO = -1,
    NES_ERR_FORMAT = -2,        /* not a valid iNES image */
    NES_ERR_RANGE = -3,         /* offset/length outside the image or region */
    NES_ERR_TOO_LONG = -4,      /* replacement exceeds the slot length */
    NES_ERR_MISMATCH = -5,      /* a `find` guard did not match */
    NES_ERR_ENCODE = -6,        /* a glyph has no byte in the active table */
    NES_ERR_ARG = -7            /* bad argument */
} nes_status;

/* An in-memory iNES image. Owns `data`. */
typedef struct {
    uint8_t *data;      /* full file bytes (header + banks)                 */
    size_t   size;      /* total byte count                                 */
    uint8_t  prg_banks; /* 16 KiB units                                     */
    uint8_t  chr_banks; /* 8 KiB units                                      */
    int      has_trainer;
    size_t   prg_offset; /* absolute file offset of PRG-ROM                 */
    size_t   prg_size;   /* PRG-ROM byte count                              */
    size_t   chr_offset; /* absolute file offset of CHR-ROM (0 if none)     */
    size_t   chr_size;   /* CHR-ROM byte count                              */
    unsigned mapper;     /* resolved mapper number                          */
} nes_rom;

/* A character map (.tbl): byte<->glyph in both directions. ASCII if unloaded. */
typedef struct nes_table nes_table;

/* ---- ROM lifecycle --------------------------------------------------- */

/* Load an iNES image from a file. Fills `rom`. Caller must nes_rom_free. */
nes_status nes_rom_load(const char *path, nes_rom *rom);

/* Write the (possibly edited) image back to a file. */
nes_status nes_rom_save(const nes_rom *rom, const char *path);

/* Release the memory owned by `rom`. */
void nes_rom_free(nes_rom *rom);

/* ---- character tables ------------------------------------------------ */

/* Create an empty (ASCII-identity) table. Caller must nes_table_free. */
nes_table *nes_table_new(void);

/* Load a .tbl character map (HH=glyph lines). */
nes_status nes_table_load(nes_table *tbl, const char *path);

void nes_table_free(nes_table *tbl);

/* ---- text / menu editing --------------------------------------------- */

/*
 * Decode `len` bytes at absolute file `offset` into `out` using `tbl`
 * (NULL = ASCII). `out` must hold at least `len+1` bytes; it is NUL-terminated.
 */
nes_status nes_decode(const nes_rom *rom, const nes_table *tbl,
                      size_t offset, size_t len, char *out);

/*
 * Replace the `len`-byte slot at `offset` with `text` encoded via `tbl`.
 * The encoded form must be <= len; the remainder is padded with the table's
 * fill byte (space, or 0x00 for ASCII). Edits `rom->data` in place.
 */
nes_status nes_set_text(nes_rom *rom, const nes_table *tbl,
                        size_t offset, size_t len, const char *text);

/*
 * Guard: succeed only if the slot at `offset` currently decodes to exactly
 * `expected`. Used to make an edit abort if the offset is wrong.
 */
nes_status nes_expect_text(const nes_rom *rom, const nes_table *tbl,
                           size_t offset, size_t len, const char *expected);

/* Human-readable message for a status code. */
const char *nes_strerror(nes_status s);

#ifdef __cplusplus
}
#endif

#endif /* NES_ROM_H */
