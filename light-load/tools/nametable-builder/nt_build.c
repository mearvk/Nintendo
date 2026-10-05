/* ============================================================================
 * nt_build — original NES nametable builder.
 * ----------------------------------------------------------------------------
 * Our own code. Reads a text screen (rows of tile indices) and emits a 1 KiB
 * nametable image (960 tile bytes + 64 attribute bytes). The companion to
 * chr-encoder: chr-encoder makes the tiles, this arranges them on screen.
 *
 * Input: up to 30 rows of up to 32 whitespace-separated tile indices (0-255).
 * Missing cells default to 0. '#' comment to end of line.
 *
 * Output: exactly 1024 bytes (960 name table + 64 attribute, zeroed attrs).
 *
 * Usage:  nt_build <screen.txt> <out.nam>
 * ========================================================================== */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NT_W 32
#define NT_H 30

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: nt_build <screen.txt> <out.nam>\n"); return 2; }
    FILE *in = fopen(argv[1], "r"); if (!in) { perror("open"); return 1; }

    uint8_t nt[1024]; memset(nt, 0, sizeof(nt));
    char line[512];
    int row = 0;
    while (fgets(line, sizeof(line), in) && row < NT_H) {
        char *h = strchr(line, '#'); if (h) *h = 0;
        int col = 0;
        char *tok = strtok(line, " \t\r\n");
        while (tok && col < NT_W) {
            int v = atoi(tok);
            nt[row * NT_W + col] = (uint8_t)(v & 0xFF);
            ++col; tok = strtok(NULL, " \t\r\n");
        }
        /* only advance a row if the line had content */
        if (col > 0) ++row;
    }
    fclose(in);

    FILE *out = fopen(argv[2], "wb"); if (!out) { perror("out"); return 1; }
    fwrite(nt, 1, sizeof(nt), out);   /* 960 name + 64 attr (attrs zero) */
    fclose(out);
    printf("built nametable (%d rows) -> %s (1024 bytes)\n", row, argv[2]);
    return 0;
}
