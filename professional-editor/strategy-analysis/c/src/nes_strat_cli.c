/*
 * nes_strat_cli.c -- NES strategy analysis CLI (C).
 * Part of the mearvk/Nintendo Professional Editor.
 *
 * Usage:
 *   nes-strat static   <rom.nes> <out.map.tsv>
 *   nes-strat evidence <trace.tsv> <out.evidence.md> [rom.nes]
 *   nes-strat full      <rom.nes> <trace.tsv> <out-prefix>
 *
 * `static`   : Part A -- code/data + jump-table map of a ROM you own.
 * `evidence` : Part B -- strategy evidence from an execution trace.
 * `full`     : A then B, cross-referencing the static map.
 */
#include "nes_strategy.h"

#include <stdio.h>
#include <string.h>

static void usage(void) {
    fprintf(stderr,
        "nes-strat -- NES ROM strategy analysis\n"
        "usage:\n"
        "  nes-strat static   <rom.nes> <out.map.tsv>\n"
        "  nes-strat evidence <trace.tsv> <out.evidence.md> [rom.nes]\n"
        "  nes-strat full      <rom.nes> <trace.tsv> <out-prefix>\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 2; }

    if (strcmp(argv[1], "static") == 0 && argc >= 4) {
        nesa_static s;
        nesa_status rc = nesa_analyze_file(argv[2], &s);
        if (rc != NESA_OK) { fprintf(stderr, "static: %s\n", nesa_strerror(rc)); return 1; }
        rc = nesa_static_write_tsv(&s, argv[3]);
        if (rc == NESA_OK) {
            size_t code = 0, jt = 0;
            for (size_t i = 0; i < s.count; i++) {
                if (s.spans[i].kind == NESA_SPAN_CODE) code++;
                else if (s.spans[i].kind == NESA_SPAN_JUMPTABLE) jt++;
            }
            printf("static: reset=%04X nmi=%04X irq=%04X; %zu code blocks, %zu jump tables -> %s\n",
                   s.reset_vec, s.nmi_vec, s.irq_vec, code, jt, argv[3]);
        } else fprintf(stderr, "static: %s\n", nesa_strerror(rc));
        nesa_static_free(&s);
        return rc == NESA_OK ? 0 : 1;
    }

    if (strcmp(argv[1], "evidence") == 0 && argc >= 4) {
        nesa_static s; nesa_static *sp = NULL;
        if (argc >= 5 && nesa_analyze_file(argv[4], &s) == NESA_OK) sp = &s;
        nesa_status rc = nesa_evidence_from_trace(argv[2], sp, argv[3]);
        if (sp) nesa_static_free(sp);
        if (rc != NESA_OK) { fprintf(stderr, "evidence: %s\n", nesa_strerror(rc)); return 1; }
        printf("evidence: %s\n", argv[3]);
        return 0;
    }

    if (strcmp(argv[1], "full") == 0 && argc >= 5) {
        char mapp[512], evp[512];
        snprintf(mapp, sizeof(mapp), "%s.map.tsv", argv[4]);
        snprintf(evp, sizeof(evp), "%s.evidence.md", argv[4]);
        nesa_static s;
        nesa_status rc = nesa_analyze_file(argv[2], &s);
        if (rc != NESA_OK) { fprintf(stderr, "full: %s\n", nesa_strerror(rc)); return 1; }
        nesa_static_write_tsv(&s, mapp);
        rc = nesa_evidence_from_trace(argv[3], &s, evp);
        nesa_static_free(&s);
        if (rc != NESA_OK) { fprintf(stderr, "full: %s\n", nesa_strerror(rc)); return 1; }
        printf("full: %s + %s\n", mapp, evp);
        return 0;
    }

    usage();
    return 2;
}
