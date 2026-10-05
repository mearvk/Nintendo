/* ============================================================================
 * pal_build — original NES palette builder/validator.
 * ----------------------------------------------------------------------------
 * Our own code. Reads up to 32 palette entries (NES master-palette indices,
 * 0x00-0x3F) and emits the 32-byte palette image the PPU driver uploads to
 * $3F00. Validates that every entry is a legal 6-bit index.
 *
 * Input: whitespace/newline separated hex or decimal values; '#' comments.
 *        Fewer than 32 entries are zero-padded; more than 32 is an error.
 *
 * Output: exactly 32 bytes.
 *
 * Usage:  pal_build <palette.txt> <out.pal>
 * ========================================================================== */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_val(const char *s, int *ok) {
    *ok = 1;
    if (s[0] == '$') return (int)strtol(s + 1, NULL, 16);
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) return (int)strtol(s + 2, NULL, 16);
    if (isdigit((unsigned char)s[0])) return atoi(s);
    *ok = 0; return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: pal_build <palette.txt> <out.pal>\n"); return 2; }
    FILE *in = fopen(argv[1], "r"); if (!in) { perror("open"); return 1; }

    uint8_t pal[32]; memset(pal, 0, sizeof(pal));
    int n = 0;
    char line[256];
    while (fgets(line, sizeof(line), in)) {
        char *h = strchr(line, '#'); if (h) *h = 0;
        char *tok = strtok(line, " \t\r\n");
        while (tok) {
            if (n >= 32) { fprintf(stderr, "error: more than 32 palette entries\n"); fclose(in); return 1; }
            int ok; int v = parse_val(tok, &ok);
            if (!ok) { fprintf(stderr, "error: bad palette value '%s'\n", tok); fclose(in); return 1; }
            if (v < 0x00 || v > 0x3F) {
                fprintf(stderr, "error: entry %d = $%02X out of range (0x00-0x3F)\n", n, v);
                fclose(in); return 1;
            }
            pal[n++] = (uint8_t)v;
            tok = strtok(NULL, " \t\r\n");
        }
    }
    fclose(in);

    FILE *out = fopen(argv[2], "wb"); if (!out) { perror("out"); return 1; }
    fwrite(pal, 1, sizeof(pal), out);
    fclose(out);
    printf("built palette (%d entries, zero-padded to 32) -> %s\n", n, argv[2]);
    return 0;
}
