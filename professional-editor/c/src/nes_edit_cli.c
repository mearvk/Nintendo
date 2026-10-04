/*
 * nes_edit_cli.c -- command-line NES ROM text/menu editor (C).
 * Part of the mearvk/Nintendo Professional Editor.
 *
 * Usage:
 *   nes-edit info   <rom.nes>
 *   nes-edit dump   <rom.nes> <offset-hex> <len> [table.tbl]
 *   nes-edit apply  <in.nes> <script.edit> <out.nes>
 *
 * The edit script grammar (shared by all implementations):
 *   # comment
 *   table <path.tbl>
 *   find  <offset-hex> <len> <text>
 *   set   <offset-hex> <len> <text>
 *   menu  <name> <offset-hex> <len> <text>
 */
#include "nes_rom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long parse_hex(const char *s) { return strtol(s, NULL, 16); }

static int cmd_info(const char *path) {
    nes_rom rom;
    nes_status s = nes_rom_load(path, &rom);
    if (s != NES_OK) { fprintf(stderr, "info: %s: %s\n", path, nes_strerror(s)); return 1; }
    printf("iNES image: %s\n", path);
    printf("  size        : %zu bytes\n", rom.size);
    printf("  PRG banks   : %u (%zu bytes @ 0x%zX)\n", rom.prg_banks, rom.prg_size, rom.prg_offset);
    printf("  CHR banks   : %u (%zu bytes @ 0x%zX)\n", rom.chr_banks, rom.chr_size, rom.chr_offset);
    printf("  mapper      : %u\n", rom.mapper);
    printf("  trainer     : %s\n", rom.has_trainer ? "yes" : "no");
    nes_rom_free(&rom);
    return 0;
}

static int cmd_dump(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "dump: need <rom> <offset-hex> <len> [table]\n"); return 2; }
    nes_rom rom;
    nes_status s = nes_rom_load(argv[2], &rom);
    if (s != NES_OK) { fprintf(stderr, "dump: %s\n", nes_strerror(s)); return 1; }
    nes_table *tbl = NULL;
    if (argc >= 6) {
        tbl = nes_table_new();
        if (nes_table_load(tbl, argv[5]) != NES_OK)
            fprintf(stderr, "dump: warning: could not load table %s (using ASCII)\n", argv[5]);
    }
    size_t off = (size_t)parse_hex(argv[3]);
    size_t len = (size_t)strtoul(argv[4], NULL, 10);
    char *buf = (char *)malloc(len + 1);
    s = nes_decode(&rom, tbl, off, len, buf);
    if (s == NES_OK) printf("0x%zX [%zu] = \"%s\"\n", off, len, buf);
    else fprintf(stderr, "dump: %s\n", nes_strerror(s));
    free(buf);
    nes_table_free(tbl);
    nes_rom_free(&rom);
    return s == NES_OK ? 0 : 1;
}

/* Split a line into up to `max` whitespace tokens; a quoted final text token
 * keeps its inner spaces. Returns token count. Modifies `line`. */
static int tokenize(char *line, char **tok, int max, int text_from) {
    int n = 0;
    char *p = line;
    while (*p && n < max) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (n == text_from && (*p == '"')) {         /* quoted text token */
            p++;
            tok[n++] = p;
            char *q = strchr(p, '"');
            if (q) { *q = '\0'; p = q + 1; }
            else { p += strlen(p); }
            break;
        }
        if (n == text_from) { tok[n++] = p; break; }  /* rest-of-line text */
        tok[n++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
    }
    return n;
}

static int cmd_apply(const char *in, const char *script, const char *out) {
    nes_rom rom;
    nes_status s = nes_rom_load(in, &rom);
    if (s != NES_OK) { fprintf(stderr, "apply: %s: %s\n", in, nes_strerror(s)); return 1; }
    FILE *f = fopen(script, "rb");
    if (!f) { fprintf(stderr, "apply: cannot open script %s\n", script); nes_rom_free(&rom); return 1; }

    nes_table *tbl = NULL;
    char line[1024];
    int lineno = 0, edits = 0, rc = 0;
    while (fgets(line, sizeof(line), f)) {
        lineno++;
        char *nl = strpbrk(line, "\r\n");
        if (nl) *nl = '\0';
        char *h = line; while (*h == ' ' || *h == '\t') h++;
        if (*h == '\0' || *h == '#') continue;

        char *tok[6] = {0};
        if (strncmp(h, "table", 5) == 0 && (h[5] == ' ' || h[5] == '\t')) {
            tokenize(h, tok, 2, 1);
            if (!tbl) tbl = nes_table_new();
            if (nes_table_load(tbl, tok[1]) != NES_OK)
                fprintf(stderr, "apply:%d: warning: table %s not loaded\n", lineno, tok[1]);
            continue;
        }
        if (strncmp(h, "find", 4) == 0) {
            int n = tokenize(h, tok, 4, 3);
            if (n < 4) { fprintf(stderr, "apply:%d: find needs offset len text\n", lineno); rc = 1; break; }
            s = nes_expect_text(&rom, tbl, (size_t)parse_hex(tok[1]), (size_t)strtoul(tok[2], NULL, 10), tok[3]);
            if (s != NES_OK) { fprintf(stderr, "apply:%d: find failed: %s\n", lineno, nes_strerror(s)); rc = 1; break; }
            continue;
        }
        if (strncmp(h, "set", 3) == 0) {
            int n = tokenize(h, tok, 4, 3);
            if (n < 4) { fprintf(stderr, "apply:%d: set needs offset len text\n", lineno); rc = 1; break; }
            s = nes_set_text(&rom, tbl, (size_t)parse_hex(tok[1]), (size_t)strtoul(tok[2], NULL, 10), tok[3]);
            if (s != NES_OK) { fprintf(stderr, "apply:%d: set failed: %s\n", lineno, nes_strerror(s)); rc = 1; break; }
            edits++;
            continue;
        }
        if (strncmp(h, "menu", 4) == 0) {
            int n = tokenize(h, tok, 5, 4);   /* menu <name> <off> <len> <text> */
            if (n < 5) { fprintf(stderr, "apply:%d: menu needs name offset len text\n", lineno); rc = 1; break; }
            s = nes_set_text(&rom, tbl, (size_t)parse_hex(tok[2]), (size_t)strtoul(tok[3], NULL, 10), tok[4]);
            if (s != NES_OK) { fprintf(stderr, "apply:%d: menu '%s' failed: %s\n", lineno, tok[1], nes_strerror(s)); rc = 1; break; }
            printf("menu '%s' set @ %s\n", tok[1], tok[2]);
            edits++;
            continue;
        }
        fprintf(stderr, "apply:%d: unknown directive\n", lineno);
        rc = 1; break;
    }
    fclose(f);
    if (rc == 0) {
        s = nes_rom_save(&rom, out);
        if (s != NES_OK) { fprintf(stderr, "apply: save: %s\n", nes_strerror(s)); rc = 1; }
        else printf("applied %d edit(s) -> %s\n", edits, out);
    } else {
        fprintf(stderr, "apply: aborted; %s not written\n", out);
    }
    nes_table_free(tbl);
    nes_rom_free(&rom);
    return rc;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
            "nes-edit -- NES ROM text/menu editor\n"
            "usage:\n"
            "  nes-edit info  <rom.nes>\n"
            "  nes-edit dump  <rom.nes> <offset-hex> <len> [table.tbl]\n"
            "  nes-edit apply <in.nes> <script.edit> <out.nes>\n");
        return 2;
    }
    if (strcmp(argv[1], "info") == 0 && argc >= 3) return cmd_info(argv[2]);
    if (strcmp(argv[1], "dump") == 0) return cmd_dump(argc, argv);
    if (strcmp(argv[1], "apply") == 0 && argc >= 5) return cmd_apply(argv[2], argv[3], argv[4]);
    fprintf(stderr, "nes-edit: unknown or incomplete command\n");
    return 2;
}
