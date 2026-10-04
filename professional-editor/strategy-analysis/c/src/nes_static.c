/*
 * nes_static.c -- Part A: static 6502 structure analysis (C).
 * Part of the mearvk/Nintendo Professional Editor. No copyrighted content.
 *
 * Classifies PRG-ROM bytes as code or data by following control flow from the
 * reset/NMI/IRQ vectors (recursive descent + per-byte visited marks), then
 * scans for jump tables: runs of little-endian pointers into the code region.
 *
 * This is a static *approximation*: indirect/computed jumps (JMP (ind), RTS
 * dispatch, bank switches) cannot be fully resolved statically. The result is
 * a map of where decision logic plausibly lives, not a guarantee. See
 * ../docs/METHOD.md.
 */
#include "nes_strategy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NES_HEADER 16u
#define PRG_BANK 16384u

/* 6502 opcode -> instruction length (1..3). 0 marks an undefined/undocumented
 * opcode, which we treat as a decode stop (1 byte, no fall-through follow). */
static const uint8_t OPLEN[256] = {
/*       x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF */
/*0x*/    1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 0, 3, 3, 0,
/*1x*/    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*2x*/    3, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*3x*/    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*4x*/    1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*5x*/    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*6x*/    1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*7x*/    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*8x*/    0, 2, 0, 0, 2, 2, 2, 0, 1, 0, 1, 0, 3, 3, 3, 0,
/*9x*/    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 0, 3, 0, 0,
/*Ax*/    2, 2, 2, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*Bx*/    2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
/*Cx*/    2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*Dx*/    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0,
/*Ex*/    2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
/*Fx*/    2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0
};

/* Opcode predicates (documented 6502). */
static int is_jmp_abs(uint8_t op)   { return op == 0x4C; }
static int is_jsr(uint8_t op)       { return op == 0x20; }
static int is_rts_rti(uint8_t op)   { return op == 0x60 || op == 0x40; }
static int is_jmp_ind(uint8_t op)   { return op == 0x6C; }
static int is_branch(uint8_t op)    { /* conditional relative branches */
    switch (op) { case 0x10: case 0x30: case 0x50: case 0x70:
                  case 0x90: case 0xB0: case 0xD0: case 0xF0: return 1; }
    return 0;
}

/* Per-PRG-byte visited marks for the recursive-descent pass. */
typedef struct {
    uint8_t *rom;      /* full iNES file */
    size_t   size;
    size_t   prg_off, prg_size;
    uint16_t map_base;
    uint8_t *code;     /* 1 if byte is a reached opcode start / operand */
} ctx;

/* Map a CPU address to a PRG-RELATIVE offset (0..prg_size) for the flat 16/32
 * KiB window; -1 if outside. This index addresses both c->code[] and, after
 * adding c->prg_off, the raw ROM bytes. */
static long cpu_to_prg(const ctx *c, uint16_t addr) {
    if (addr < c->map_base) return -1;
    size_t off = (size_t)(addr - c->map_base);
    /* 16 KiB ROMs are mirrored into both $8000 and $C000. */
    if (c->prg_size == PRG_BANK) off %= PRG_BANK;
    if (off >= c->prg_size) return -1;
    return (long)off;
}

/* Read a little-endian word at a PRG-RELATIVE offset. */
static uint16_t rd16(const ctx *c, size_t prg_rel) {
    size_t fo = c->prg_off + prg_rel;
    return (uint16_t)(c->rom[fo] | (c->rom[fo + 1] << 8));
}

/* Recursive descent from one entry CPU address. Iterative worklist to avoid
 * deep recursion; follows fall-through, branches, JMP abs, and JSR targets. */
static void trace_from(ctx *c, uint16_t entry) {
    enum { MAXW = 1u << 16 };
    static uint16_t stack[MAXW];
    size_t sp = 0;
    stack[sp++] = entry;
    while (sp) {
        uint16_t pc = stack[--sp];
        for (;;) {
            long off = cpu_to_prg(c, pc);           /* PRG-relative index    */
            if (off < 0) break;                     /* left the ROM window   */
            if (c->code[off]) break;                /* already visited       */
            uint8_t op = c->rom[c->prg_off + off];
            uint8_t len = OPLEN[op];
            if (len == 0) { c->code[off] = 1; break; } /* undefined: stop     */
            /* mark opcode + operand bytes */
            for (uint8_t i = 0; i < len; i++) {
                long o = cpu_to_prg(c, (uint16_t)(pc + i));
                if (o >= 0) c->code[o] = 1;
            }
            if (is_jsr(op) || is_jmp_abs(op)) {
                uint16_t tgt = rd16(c, (size_t)off + 1);
                if (sp < MAXW) stack[sp++] = tgt;
                if (is_jmp_abs(op)) break;          /* no fall-through       */
            } else if (is_branch(op)) {
                int8_t rel = (int8_t)c->rom[c->prg_off + off + 1];
                uint16_t tgt = (uint16_t)(pc + 2 + rel);
                if (sp < MAXW) stack[sp++] = tgt;   /* taken path            */
                /* fall through to not-taken path below */
            } else if (is_rts_rti(op) || is_jmp_ind(op)) {
                break;                              /* flow leaves statically */
            }
            pc = (uint16_t)(pc + len);              /* fall-through          */
        }
    }
}

/* Append a span to the result. */
static nesa_status push_span(nesa_static *s, nesa_span_kind k, uint16_t addr,
                             size_t prg_off, size_t len, unsigned detail) {
    if (s->count == s->cap) {
        size_t nc = s->cap ? s->cap * 2 : 64;
        nesa_span *np = (nesa_span *)realloc(s->spans, nc * sizeof(*np));
        if (!np) return NESA_ERR_MEM;
        s->spans = np; s->cap = nc;
    }
    s->spans[s->count].kind = k;
    s->spans[s->count].cpu_addr = addr;
    s->spans[s->count].prg_off = prg_off;
    s->spans[s->count].len = len;
    s->spans[s->count].detail = detail;
    s->count++;
    return NESA_OK;
}

/*
 * Detect jump tables: runs of >= MIN_ENTRIES consecutive little-endian words
 * that all point into the PRG window (a plausible address table). This is the
 * classic decision-dispatch structure, and crucially its targets are often
 * only reachable *through* the table (indirect dispatch), so we also SEED the
 * code pass from each entry via `seed` -- resolving the chicken-and-egg where a
 * jump table points at code nothing branches to statically.
 *
 * `seed` is called for every table entry so the caller can run trace_from on
 * each target; pass NULL to only record tables.
 */
static void detect_jumptables(ctx *c, nesa_static *s, void (*seed)(ctx *, uint16_t)) {
    const unsigned MIN_ENTRIES = 4;
    size_t i = 0;
    while (i + 2 <= c->prg_size) {
        size_t start = i, entries = 0;
        while (i + 2 <= c->prg_size) {
            uint16_t ptr = rd16(c, i);             /* PRG-relative word      */
            long tgt = cpu_to_prg(c, ptr);
            if (tgt < 0) break;                     /* not into the PRG window */
            entries++; i += 2;
        }
        if (entries >= MIN_ENTRIES) {
            uint16_t addr = (uint16_t)(c->map_base + (start % (c->prg_size == PRG_BANK ? PRG_BANK : c->prg_size)));
            if (s) push_span(s, NESA_SPAN_JUMPTABLE, addr, c->prg_off + start,
                             entries * 2, (unsigned)entries);
            if (seed) {
                for (size_t e = 0; e < entries; e++)
                    seed(c, rd16(c, start + e * 2));
            }
        } else {
            i = start + 1;                          /* advance by one byte    */
        }
    }
}

/* Emit code/data spans by scanning the per-byte code marks into runs. */
static void emit_spans(const ctx *c, nesa_static *s) {
    size_t i = 0;
    while (i < c->prg_size) {
        uint8_t v = c->code[i];
        size_t run = 1;
        while (i + run < c->prg_size && c->code[i + run] == v) run++;
        uint16_t addr = (uint16_t)(c->map_base + (i % (c->prg_size == PRG_BANK ? PRG_BANK : c->prg_size)));
        push_span(s, v ? NESA_SPAN_CODE : NESA_SPAN_DATA, addr, c->prg_off + i, run, 0);
        i += run;
    }
}

nesa_status nesa_analyze_file(const char *path, nesa_static *out) {
    if (!path || !out) return NESA_ERR_ARG;
    memset(out, 0, sizeof(*out));

    FILE *f = fopen(path, "rb");
    if (!f) return NESA_ERR_IO;
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    if (sz < (long)NES_HEADER) { fclose(f); return NESA_ERR_FORMAT; }
    uint8_t *rom = (uint8_t *)malloc((size_t)sz);
    if (!rom) { fclose(f); return NESA_ERR_MEM; }
    if (fread(rom, 1, (size_t)sz, f) != (size_t)sz) { free(rom); fclose(f); return NESA_ERR_IO; }
    fclose(f);
    if (memcmp(rom, "NES\x1A", 4) != 0) { free(rom); return NESA_ERR_FORMAT; }

    unsigned prg_banks = rom[4];
    int has_trainer = (rom[6] & 0x04) ? 1 : 0;
    size_t prg_off = NES_HEADER + (has_trainer ? 512u : 0u);
    size_t prg_size = (size_t)prg_banks * PRG_BANK;
    if (prg_off + prg_size > (size_t)sz) { free(rom); return NESA_ERR_FORMAT; }

    ctx c;
    c.rom = rom; c.size = (size_t)sz;
    c.prg_off = prg_off; c.prg_size = prg_size;
    /* A 16 KiB PRG maps at $C000 (mirrored to $8000); larger maps at $8000. */
    c.map_base = (prg_size == PRG_BANK) ? 0xC000 : 0x8000;
    c.code = (uint8_t *)calloc(prg_size ? prg_size : 1, 1);
    if (!c.code) { free(rom); return NESA_ERR_MEM; }

    /* Vectors live in the last 6 bytes of the CPU space, i.e. end of PRG.
     * rd16 takes a PRG-RELATIVE offset. */
    size_t vec = prg_size - 6;
    out->nmi_vec   = rd16(&c, vec + 0);
    out->reset_vec = rd16(&c, vec + 2);
    out->irq_vec   = rd16(&c, vec + 4);
    out->prg_size  = prg_size;
    out->map_base  = c.map_base;

    /* 1) Seed the code pass from the hardware vectors. */
    trace_from(&c, out->reset_vec);
    trace_from(&c, out->nmi_vec);
    trace_from(&c, out->irq_vec);

    /* 2) Discover jump tables and SEED the code pass from their entries, so
     *    indirectly-dispatched routines (reachable only through a table) are
     *    classified as code too. Iterate to a fixpoint: newly-traced code can
     *    expose further tables. */
    size_t prev_marks;
    do {
        prev_marks = 0;
        for (size_t k = 0; k < c.prg_size; k++) prev_marks += c.code[k];
        detect_jumptables(&c, NULL, trace_from);   /* seed only, don't record */
        size_t now = 0;
        for (size_t k = 0; k < c.prg_size; k++) now += c.code[k];
        if (now == prev_marks) break;
    } while (1);

    /* 3) Record the jump tables (now that targets are marked) and emit spans. */
    detect_jumptables(&c, out, NULL);
    emit_spans(&c, out);

    free(c.code);
    free(rom);
    return NESA_OK;
}

nesa_status nesa_static_write_tsv(const nesa_static *s, const char *path) {
    if (!s || !path) return NESA_ERR_ARG;
    FILE *f = fopen(path, "wb");
    if (!f) return NESA_ERR_IO;
    fprintf(f, "# NES static map (code-vs-data + jump tables). Approximate; see METHOD.md.\n");
    fprintf(f, "# vectors: reset=%04X nmi=%04X irq=%04X  prg=%zu map_base=%04X\n",
            s->reset_vec, s->nmi_vec, s->irq_vec, s->prg_size, s->map_base);
    fprintf(f, "kind\tbank\taddr\tend\tdetail\n");
    fprintf(f, "vector\t-\tFFFC\tFFFD\treset=%04X\n", s->reset_vec);
    for (size_t i = 0; i < s->count; i++) {
        const nesa_span *sp = &s->spans[i];
        const char *k = sp->kind == NESA_SPAN_CODE ? "code" :
                        sp->kind == NESA_SPAN_JUMPTABLE ? "jumptable" : "data";
        unsigned end = (unsigned)(sp->cpu_addr + (sp->len ? sp->len - 1 : 0));
        if (sp->kind == NESA_SPAN_JUMPTABLE)
            fprintf(f, "%s\t0\t%04X\t%04X\tentries=%u\n", k, sp->cpu_addr, end, sp->detail);
        else
            fprintf(f, "%s\t0\t%04X\t%04X\t%s\n", k, sp->cpu_addr, end,
                    sp->kind == NESA_SPAN_CODE ? "block" : "unreached");
    }
    fclose(f);
    return NESA_OK;
}

void nesa_static_free(nesa_static *s) {
    if (s && s->spans) { free(s->spans); s->spans = NULL; s->count = s->cap = 0; }
}

const char *nesa_strerror(nesa_status s) {
    switch (s) {
        case NESA_OK:          return "ok";
        case NESA_ERR_IO:      return "I/O error";
        case NESA_ERR_FORMAT:  return "not a valid iNES image";
        case NESA_ERR_ARG:     return "bad argument";
        case NESA_ERR_MEM:     return "out of memory";
        default:               return "unknown error";
    }
}
