/* ============================================================================
 * rom_split — "unlink": split a .nes we built back into header / PRG / CHR.
 * ----------------------------------------------------------------------------
 * Our own code. The inverse of tools/rom-linker: given a .nes image, it writes
 * three files — <out>.hdr (16 bytes), <out>.prg, <out>.chr — so we can inspect
 * or re-process the parts of OUR OWN build. It also prints the vector table.
 *
 * It operates on bytes we hand it and does not interpret game content.
 *
 * Usage:
 *   rom_split <in.nes> <out_prefix>
 * ========================================================================== */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRG_BANK 16384u
#define CHR_BANK  8192u

static int write_file(const char *path, const uint8_t *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }
    if (len) fwrite(data, 1, len, f);
    fclose(f);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: rom_split <in.nes> <out_prefix>\n"); return 2; }

    FILE *fp = fopen(argv[1], "rb"); if (!fp) { perror("open"); return 1; }
    fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
    if (n < 16) { fclose(fp); fprintf(stderr, "not a .nes (too small)\n"); return 1; }
    uint8_t *b = malloc((size_t)n);
    if (fread(b, 1, (size_t)n, fp) != (size_t)n) { fclose(fp); free(b); return 1; }
    fclose(fp);

    if (!(b[0]=='N'&&b[1]=='E'&&b[2]=='S'&&b[3]==0x1A)) {
        fprintf(stderr, "missing NES\\x1A magic\n"); free(b); return 1;
    }

    int prg_banks = b[4];
    int chr_banks = b[5];
    int mapper = (b[6] >> 4) | (b[7] & 0xF0);
    unsigned prg_bytes = (unsigned)prg_banks * PRG_BANK;
    unsigned chr_bytes = (unsigned)chr_banks * CHR_BANK;
    unsigned trainer = (b[6] & 0x04) ? 512u : 0u;

    unsigned prg_off = 16u + trainer;
    unsigned chr_off = prg_off + prg_bytes;

    if (prg_off + prg_bytes > (unsigned)n) {
        fprintf(stderr, "warning: PRG region exceeds file; clamping\n");
        prg_bytes = (unsigned)n - prg_off;
    }
    if (chr_off + chr_bytes > (unsigned)n) {
        if (chr_off < (unsigned)n) chr_bytes = (unsigned)n - chr_off;
        else chr_bytes = 0;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s.hdr", argv[2]);
    if (write_file(path, b, 16)) { free(b); return 1; }
    snprintf(path, sizeof(path), "%s.prg", argv[2]);
    if (write_file(path, b + prg_off, prg_bytes)) { free(b); return 1; }
    if (chr_bytes) {
        snprintf(path, sizeof(path), "%s.chr", argv[2]);
        if (write_file(path, b + chr_off, chr_bytes)) { free(b); return 1; }
    }

    printf("split %s: mapper %d, %d PRG bank(s) (%u B), %d CHR bank(s) (%u B)\n",
           argv[1], mapper, prg_banks, prg_bytes, chr_banks, chr_bytes);

    /* vectors: last 6 bytes of the PRG region */
    if (prg_bytes >= 6) {
        const uint8_t *v = b + prg_off + prg_bytes - 6;
        unsigned nmi   = v[0] | (v[1] << 8);
        unsigned reset = v[2] | (v[3] << 8);
        unsigned irq   = v[4] | (v[5] << 8);
        printf("  vectors: NMI=$%04X RESET=$%04X IRQ=$%04X\n", nmi, reset, irq);
    }
    printf("  wrote %s.hdr, %s.prg%s\n", argv[2], argv[2],
           chr_bytes ? " and .chr" : "");
    free(b);
    return 0;
}
