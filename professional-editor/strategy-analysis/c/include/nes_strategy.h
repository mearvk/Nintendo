/*
 * nes_strategy.h -- NES ROM strategy analysis (C API).
 *
 * Part of the mearvk/Nintendo Professional Editor. Analyzes a ROM you own to
 * produce your own structural + behavioral evidence; ships no copyrighted
 * content and copies no game code. See ../docs/METHOD.md for the honest limits.
 *
 * Two parts:
 *   A) Static structure: 6502 code-vs-data reachability from the vectors and
 *      jump-table detection over PRG-ROM.
 *   B) Dynamic evidence: ingest an execution trace and attribute exercised code
 *      to the decision context that unlocked it.
 */
#ifndef NES_STRATEGY_H
#define NES_STRATEGY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NESA_OK = 0,
    NESA_ERR_IO = -1,
    NESA_ERR_FORMAT = -2,
    NESA_ERR_ARG = -3,
    NESA_ERR_MEM = -4
} nesa_status;

/* ---- Part A: static structure ---------------------------------------- */

/* Classification of a span of PRG bytes. */
typedef enum {
    NESA_SPAN_CODE = 1,       /* reached as executable code                 */
    NESA_SPAN_DATA = 2,       /* not reached as code (data or unreached)    */
    NESA_SPAN_JUMPTABLE = 3   /* a detected address table (jump/branch)     */
} nesa_span_kind;

typedef struct {
    nesa_span_kind kind;
    uint16_t cpu_addr;  /* CPU address (0x8000..0xFFFF for a 32KiB map)     */
    size_t   prg_off;   /* offset into PRG-ROM                              */
    size_t   len;       /* bytes                                            */
    unsigned detail;    /* jumptable: entry count; else 0                  */
} nesa_span;

/* The result of a static pass over a loaded PRG-ROM image. */
typedef struct {
    nesa_span *spans;
    size_t     count;
    size_t     cap;
    uint16_t   reset_vec, nmi_vec, irq_vec;
    size_t     prg_size;
    uint16_t   map_base;   /* CPU address PRG byte 0 maps to (0x8000/0xC000) */
} nesa_static;

/*
 * Run the static pass over a raw iNES file. Classifies code/data from the
 * reset/NMI/IRQ vectors (recursive descent) and detects jump tables.
 * `out` must be freed with nesa_static_free.
 */
nesa_status nesa_analyze_file(const char *path, nesa_static *out);

/* Write the static map as TSV (see METHOD.md `*.map.tsv`). */
nesa_status nesa_static_write_tsv(const nesa_static *s, const char *path);

void nesa_static_free(nesa_static *s);

/* ---- Part B: dynamic evidence ---------------------------------------- */

/*
 * Ingest an execution trace (TSV) and write a Strategy Evidence report
 * (Markdown). Optionally cross-reference the static map (may be NULL) so the
 * report can note which exercised addresses fell in detected jump tables.
 *
 * The trace schema and report format are defined in ../docs/METHOD.md.
 */
nesa_status nesa_evidence_from_trace(const char *trace_path,
                                     const nesa_static *static_map_or_null,
                                     const char *out_md_path);

const char *nesa_strerror(nesa_status s);

#ifdef __cplusplus
}
#endif

#endif /* NES_STRATEGY_H */
