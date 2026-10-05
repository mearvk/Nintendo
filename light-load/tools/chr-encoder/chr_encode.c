/* ============================================================================
 * chr_encode — original NES CHR tile encoder.
 * ----------------------------------------------------------------------------
 * Our own code. Reads a simple text description of 8x8 tiles (one 2-bit
 * palette index per pixel, 0..3) and packs them into the NES two-bit-plane
 * CHR format (16 bytes per tile): plane 0 (low bits) then plane 1 (high bits),
 * row by row. Not derived from any third-party editor.
 *
 * Input format (text):
 *   - lines of exactly 8 digits (0-3), 8 lines per tile
 *   - blank lines separate tiles (optional)
 *   - '#' starts a comment to end of line
 *
 * Usage:
 *   chr_encode <tiles.txt> <out.chr>
 * ============================================================================ */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: chr_encode <tiles.txt> <out.chr>\n"); return 2; }
    FILE *in = fopen(argv[1], "r");
    if (!in) { perror("open"); return 1; }
    FILE *out = fopen(argv[2], "wb");
    if (!out) { perror("out"); fclose(in); return 1; }

    char line[256];
    uint8_t rows[8];        /* current tile: 8 rows of 2-bit indices */
    int rowcount = 0;
    int tiles = 0;
    long bytes = 0;

    /* We buffer per-tile; emit plane0 (8 bytes) then plane1 (8 bytes). */
    uint8_t plane0[8], plane1[8];

    while (fgets(line, sizeof(line), in)) {
        /* strip comment + whitespace */
        char *h = strchr(line, '#'); if (h) *h = 0;
        char buf[16]; int n = 0;
        for (char *p = line; *p && n < 15; ++p)
            if (*p >= '0' && *p <= '3') buf[n++] = *p;
        buf[n] = 0;

        if (n == 0) continue;              /* blank/comment line */
        if (n != 8) {
            fprintf(stderr, "error: row with %d pixels (need 8): %s", n, line);
            fclose(in); fclose(out); return 1;
        }

        /* build the two bit-planes for this row */
        uint8_t p0 = 0, p1 = 0;
        for (int x = 0; x < 8; ++x) {
            int v = buf[x] - '0';
            p0 = (uint8_t)((p0 << 1) | (v & 1));
            p1 = (uint8_t)((p1 << 1) | ((v >> 1) & 1));
        }
        plane0[rowcount] = p0;
        plane1[rowcount] = p1;
        (void)rows;
        ++rowcount;

        if (rowcount == 8) {
            fwrite(plane0, 1, 8, out);
            fwrite(plane1, 1, 8, out);
            bytes += 16;
            ++tiles;
            rowcount = 0;
        }
    }

    if (rowcount != 0) {
        fprintf(stderr, "error: trailing partial tile (%d rows)\n", rowcount);
        fclose(in); fclose(out); return 1;
    }
    fclose(in); fclose(out);
    printf("encoded %d tiles (%ld bytes) -> %s\n", tiles, bytes, argv[2]);
    return 0;
}
