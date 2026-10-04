/*
 * nes_evidence.c -- Part B: dynamic strategy evidence (C).
 * Part of the mearvk/Nintendo Professional Editor. No copyrighted content.
 *
 * Ingests an execution trace (TSV emitted by an emulator/harness over a ROM
 * the user owns) and produces a Strategy Evidence report (Markdown): per
 * decision context, which code addresses and jump tables were exercised, with
 * coverage counts and explicit confidence caveats. See ../docs/METHOD.md.
 */
#include "nes_strategy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A small aggregation keyed by decision context. We keep it simple and
 * dependency-free: a linear list of decisions, each with hit counters and a
 * compact set of distinct exercised code addresses + jump-table addresses. */
#define MAX_DECISIONS 256
#define MAX_ADDRS 4096

typedef struct {
    char     name[64];
    unsigned execs, branches, taken, reads, writes, jtbls;
    uint16_t code_addrs[MAX_ADDRS]; size_t code_n;
    uint16_t jtbl_addrs[64];        size_t jtbl_n;
} decision_rec;

static decision_rec *find_decision(decision_rec *recs, size_t *n, const char *name) {
    for (size_t i = 0; i < *n; i++)
        if (strcmp(recs[i].name, name) == 0) return &recs[i];
    if (*n >= MAX_DECISIONS) return &recs[*n - 1]; /* clamp: fold into last */
    decision_rec *r = &recs[(*n)++];
    memset(r, 0, sizeof(*r));
    snprintf(r->name, sizeof(r->name), "%s", name);
    return r;
}

static void add_addr(uint16_t *arr, size_t *n, size_t cap, uint16_t a) {
    for (size_t i = 0; i < *n; i++) if (arr[i] == a) return;
    if (*n < cap) arr[(*n)++] = a;
}

/* Is `addr` inside a detected jump table in the static map? (optional) */
static int in_jumptable(const nesa_static *s, uint16_t addr) {
    if (!s) return 0;
    for (size_t i = 0; i < s->count; i++) {
        if (s->spans[i].kind != NESA_SPAN_JUMPTABLE) continue;
        uint16_t lo = s->spans[i].cpu_addr;
        uint16_t hi = (uint16_t)(lo + s->spans[i].len);
        if (addr >= lo && addr < hi) return 1;
    }
    return 0;
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ' || e[-1] == '\t')) *--e = '\0';
    return s;
}

/* Parse "key=value" from a token; returns value or NULL. */
static const char *kv(const char *tok, const char *key) {
    size_t kl = strlen(key);
    if (strncmp(tok, key, kl) == 0 && tok[kl] == '=') return tok + kl + 1;
    return NULL;
}

nesa_status nesa_evidence_from_trace(const char *trace_path,
                                     const nesa_static *smap,
                                     const char *out_md_path) {
    if (!trace_path || !out_md_path) return NESA_ERR_ARG;
    FILE *f = fopen(trace_path, "rb");
    if (!f) return NESA_ERR_IO;

    decision_rec *recs = (decision_rec *)calloc(MAX_DECISIONS, sizeof(*recs));
    if (!recs) { fclose(f); return NESA_ERR_MEM; }
    size_t ndec = 0;
    unsigned long total_events = 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *s = trim(line);
        if (*s == '\0' || *s == '#') continue;
        /* columns: seq  event  pc  arg  decision   (tab-separated) */
        char *cols[5] = {0};
        int nc = 0;
        char *p = s;
        for (; nc < 5 && p; nc++) {
            char *tab = strchr(p, '\t');
            cols[nc] = p;
            if (tab) { *tab = '\0'; p = tab + 1; } else { p = NULL; }
        }
        if (nc < 2) continue;
        const char *event = cols[1];
        const char *pcs   = nc > 2 ? cols[2] : "-";
        const char *arg   = nc > 3 ? cols[3] : "-";
        const char *dec   = nc > 4 ? cols[4] : "unknown";
        total_events++;

        decision_rec *r = find_decision(recs, &ndec, dec);
        uint16_t pc = (uint16_t)strtol(pcs, NULL, 16);

        if (strcmp(event, "exec") == 0) {
            r->execs++;
            add_addr(r->code_addrs, &r->code_n, MAX_ADDRS, pc);
        } else if (strcmp(event, "branch") == 0) {
            r->branches++;
            const char *tk = kv(arg, "taken");
            if (tk && atoi(tk) != 0) r->taken++;
        } else if (strcmp(event, "read") == 0) {
            r->reads++;
        } else if (strcmp(event, "write") == 0) {
            r->writes++;
        } else if (strcmp(event, "jtbl") == 0) {
            r->jtbls++;
            add_addr(r->jtbl_addrs, &r->jtbl_n, 64, pc);
        }
    }
    fclose(f);

    FILE *o = fopen(out_md_path, "wb");
    if (!o) { free(recs); return NESA_ERR_IO; }
    fprintf(o, "# Strategy Evidence Report\n\n");
    fprintf(o, "Source trace: `%s`  \nTotal events: %lu  \nDecision contexts: %zu\n\n",
            trace_path, total_events, ndec);
    fprintf(o, "> This report reflects only the code paths the supplied trace actually\n");
    fprintf(o, "> exercised (and, where cross-referenced, a static approximation of code).\n");
    fprintf(o, "> It is evidence for a human analysis, not a complete or guaranteed\n");
    fprintf(o, "> extraction of the game's strategy. Absence of evidence is not evidence\n");
    fprintf(o, "> of absence.\n\n");

    for (size_t i = 0; i < ndec; i++) {
        decision_rec *r = &recs[i];
        fprintf(o, "## Decision: `%s`\n\n", r->name);
        fprintf(o, "- exec events: %u (distinct code addresses: %zu)\n", r->execs, r->code_n);
        fprintf(o, "- branches: %u (taken: %u)\n", r->branches, r->taken);
        fprintf(o, "- reads: %u, writes: %u\n", r->reads, r->writes);
        fprintf(o, "- jump-table dispatches: %u (distinct tables: %zu)\n", r->jtbls, r->jtbl_n);
        if (r->jtbl_n) {
            fprintf(o, "- dispatch addresses:");
            for (size_t j = 0; j < r->jtbl_n; j++) {
                int xref = in_jumptable(smap, r->jtbl_addrs[j]);
                fprintf(o, " %04X%s", r->jtbl_addrs[j], xref ? "(static-jt)" : "");
            }
            fprintf(o, "\n");
        }
        /* Routine entry hint: lowest exercised address under this decision. */
        if (r->code_n) {
            uint16_t lo = r->code_addrs[0];
            for (size_t j = 1; j < r->code_n; j++) if (r->code_addrs[j] < lo) lo = r->code_addrs[j];
            fprintf(o, "- first exercised address under this decision: `%04X`\n", lo);
        }
        fprintf(o, "\n");
    }
    fprintf(o, "---\n");
    fprintf(o, "Method: static structure (Part A) + dynamic trace (Part B). See `docs/METHOD.md`.\n");
    fclose(o);
    free(recs);
    return NESA_OK;
}
