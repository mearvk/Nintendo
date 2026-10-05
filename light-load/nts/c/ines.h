/* ============================================================================
 * Nintendo Technical Series (NTS) — iNES structural reader
 * ----------------------------------------------------------------------------
 * This header declares a STRUCTURAL parser for the iNES / NES 2.0 container
 * format. It reads only the 16-byte header and derives the physical region
 * layout (header / trainer / PRG-ROM / CHR-ROM). It deliberately does NOT
 * interpret, extract, or reproduce any game content (text, maps, CHR tiles,
 * code). It is the "describe, don't distribute" companion to light-load.
 *
 * Format reference (public, well-documented): the iNES header is 16 bytes:
 *   [0..3] magic 'N' 'E' 'S' 0x1A
 *   [4]    PRG-ROM size in 16 KiB units
 *   [5]    CHR-ROM size in 8  KiB units (0 => the board uses CHR-RAM)
 *   [6]    flags6  : mirroring, battery, trainer, mapper low nibble
 *   [7]    flags7  : VS/PlayChoice, NES 2.0 id, mapper high nibble
 *   [8..15] size/TV/misc (interpreted per iNES vs NES 2.0)
 * ========================================================================== */
#ifndef NTS_INES_H
#define NTS_INES_H

#include <stddef.h>
#include <stdint.h>

#define NTS_INES_HEADER_SIZE   16u
#define NTS_PRG_BANK_SIZE      (16u * 1024u)
#define NTS_CHR_BANK_SIZE      ( 8u * 1024u)
#define NTS_TRAINER_SIZE       512u

typedef enum {
    NTS_OK = 0,
    NTS_ERR_IO,
    NTS_ERR_TOO_SMALL,
    NTS_ERR_BAD_MAGIC,
    NTS_ERR_SIZE_MISMATCH,
    NTS_ERR_NULL
} nts_status;

typedef enum {
    NTS_MIRROR_HORIZONTAL = 0,
    NTS_MIRROR_VERTICAL   = 1,
    NTS_MIRROR_FOUR_SCREEN = 2
} nts_mirroring;

typedef enum {
    NTS_FMT_INES    = 1,   /* archaic / standard iNES */
    NTS_FMT_NES20   = 2    /* NES 2.0 (flags7 bits 2-3 == 0b10) */
} nts_format;

/* A physical region within the file image. Offsets/lengths are byte-exact. */
typedef struct {
    const char *name;      /* "header" / "trainer" / "prg" / "chr" */
    uint32_t    offset;    /* byte offset from start of file          */
    uint32_t    length;    /* byte length                            */
} nts_region;

typedef struct {
    nts_format    format;
    uint16_t      mapper;          /* combined mapper number           */
    uint8_t       submapper;       /* NES 2.0 only (else 0)            */
    uint32_t      prg_banks;       /* count of 16 KiB PRG banks        */
    uint32_t      chr_banks;       /* count of  8 KiB CHR banks (0=RAM)*/
    uint32_t      prg_bytes;
    uint32_t      chr_bytes;
    nts_mirroring mirroring;
    int           has_battery;     /* persistent WRAM present          */
    int           has_trainer;     /* 512-byte trainer present         */
    int           uses_chr_ram;    /* chr_banks == 0                   */

    uint32_t      file_size;       /* actual size on disk              */
    uint32_t      computed_size;   /* header + trainer + prg + chr     */

    nts_region    regions[4];      /* header, [trainer], prg, [chr]    */
    uint32_t      region_count;

    uint8_t       raw_header[NTS_INES_HEADER_SIZE];
} nts_rom;

/* Parse a header buffer of at least NTS_INES_HEADER_SIZE bytes, with the
 * total file size for region/consistency computation. */
nts_status nts_parse(const uint8_t *buf, size_t buf_len,
                     uint32_t file_size, nts_rom *out);

/* Read a file from disk and parse its structure. */
nts_status nts_read_file(const char *path, nts_rom *out);

/* A lightweight additive checksum over a region, for integrity notes.
 * (Not a cryptographic hash — just a stable fingerprint of structure.) */
uint32_t   nts_region_sum(const uint8_t *data, uint32_t off, uint32_t len);

const char *nts_status_str(nts_status s);
const char *nts_mirroring_str(nts_mirroring m);

#endif /* NTS_INES_H */
