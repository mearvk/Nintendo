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

/* ---- integrity audit (recurve) & provenance refresh (refresh) -------- */

/*
 * A single finding from a `recurve` health audit. `severity` is one of the
 * NES_SEV_* values; `message` is a static or rom-lifetime string.
 */
#define NES_SEV_OK    0   /* informational: a check passed                 */
#define NES_SEV_WARN  1   /* non-fatal: unusual but structurally valid     */
#define NES_SEV_ERROR 2   /* the image is malformed or damaged             */

typedef struct {
    int  severity;          /* NES_SEV_*                                   */
    char check[16];         /* short name of the check ("header", "chr#3") */
    char message[160];
} nes_finding;

/*
 * The result of a full integrity audit. `findings[0..count)` are populated in
 * audit order; `worst` is the highest severity seen (NES_SEV_OK if all passed).
 */
#define NES_MAX_FINDINGS 128
typedef struct {
    nes_finding findings[NES_MAX_FINDINGS];
    size_t      count;
    int         worst;      /* NES_SEV_* */
} nes_report;

/*
 * recurve: audit the ROM's structural and image health without modifying it.
 *   - header soundness (magic, bank counts, mapper, trainer flag)
 *   - size consistency (header + trainer + PRG + CHR vs. file length)
 *   - per-CHR-bank "embedded image" checks: blank (all 0x00) / erased
 *     (all 0xFF) banks are flagged; each bank's checksum is recorded
 *   - PRG tail-fill observation
 * Fills `rep`. Returns NES_OK unless an argument is bad.
 */
nes_status nes_recurve(const nes_rom *rom, nes_report *rep);

/* Severity label ("ok" | "warn" | "error"). */
const char *nes_severity_str(int severity);

/*
 * A stable 64-bit content fingerprint (FNV-1a) over the whole image. Shared by
 * all four implementations so a refresh checksum is comparable across tools.
 */
uint64_t nes_fingerprint(const nes_rom *rom);

/*
 * refresh: write a clean, canonical re-emission of a ROM the user already owns
 * to `out_path` (byte-identical payload, re-serialized), and write a sidecar
 * provenance record to "<out_path>.provenance" carrying an ISO-8601 UTC refresh
 * timestamp and the image fingerprint. No ROM content is synthesized or
 * generated; the copyrighted bytes are never altered. The fingerprint, if
 * non-NULL, is returned via `out_fp`.
 */
nes_status nes_refresh(const nes_rom *rom, const char *out_path, uint64_t *out_fp);

#ifdef __cplusplus
}
#endif

#endif /* NES_ROM_H */
