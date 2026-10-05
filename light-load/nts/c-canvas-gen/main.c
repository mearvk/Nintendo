/* ============================================================================
 * NTS canvas CLI — emit a NEW, EMPTY NES 2.0 image of a chosen footprint.
 *
 *   nts-canvas <out.nes> <prg_MB> [chr_MB] [mapper] [--body]
 *
 *   prg_MB / chr_MB : requested footprint in MB (rounded up to the nearest
 *                     NES 2.0-encodable size).
 *   mapper          : mapper number to declare (default 5 / MMC5).
 *   --body          : also write the full zeroed PRG+CHR body (large!).
 *                     Without it, only the 16-byte header is written (a
 *                     capacity declaration you can inspect with nts-analyze).
 *
 * This produces original, empty bytes only. It copies nothing from any game.
 * A multi-hundred-MB canvas is a FORMAT-CAPACITY demonstration, not a runnable
 * ROM — no emulator or cart will boot it.
 * ========================================================================== */
#include "canvas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    nts_canvas_spec spec;
    uint64_t ap = 0, ac = 0;
    int body = 0, i;
    const char *out;
    double prg_mb = 0.0, chr_mb = 0.0;
    nts_canvas_status st;

    if (argc < 3) {
        fprintf(stderr,
            "usage: nts-canvas <out.nes> <prg_MB> [chr_MB] [mapper] [--body]\n");
        return 2;
    }
    out = argv[1];
    prg_mb = atof(argv[2]);

    memset(&spec, 0, sizeof(spec));
    spec.mapper = 5;        /* default MMC5 */
    spec.battery = 1;

    /* positional optionals: chr_MB, mapper; flag: --body */
    for (i = 3; i < argc; ++i) {
        if (strcmp(argv[i], "--body") == 0) { body = 1; continue; }
        if (chr_mb == 0.0 && argv[i][0] != '-') { chr_mb = atof(argv[i]); continue; }
        spec.mapper = (uint16_t)atoi(argv[i]);
    }

    spec.prg_bytes = (uint64_t)(prg_mb * 1024.0 * 1024.0);
    spec.chr_bytes = (uint64_t)(chr_mb * 1024.0 * 1024.0);

    st = nts_canvas_write(out, &spec, body, &ap, &ac);
    if (st != NTS_CANVAS_OK) {
        fprintf(stderr, "error: %s\n", nts_canvas_status_str(st));
        return 1;
    }

    printf("wrote %s\n", out);
    printf("  declared PRG : %.2f MB (requested %.2f MB)\n",
           (double)ap / (1024.0 * 1024.0), prg_mb);
    printf("  declared CHR : %.2f MB (requested %.2f MB)\n",
           (double)ac / (1024.0 * 1024.0), chr_mb);
    printf("  mapper       : %u%s\n", spec.mapper,
           spec.battery ? " (battery)" : "");
    printf("  body written : %s\n", body ? "yes (full zeroed canvas)"
                                         : "no (header-only capacity declaration)");
    printf("  note         : format-capacity canvas; not a runnable ROM.\n");
    return 0;
}
