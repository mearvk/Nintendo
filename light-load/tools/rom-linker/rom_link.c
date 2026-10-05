/* ============================================================================
 * rom_link — assemble a bootable iNES / NES 2.0 image from our own parts.
 * ----------------------------------------------------------------------------
 * Our own code. Takes a PRG blob and (optionally) a CHR blob we produced, and
 * writes a complete .nes file with a valid header AND correct 6502 interrupt
 * vectors (NMI / RESET / IRQ at $FFFA-$FFFF). Without correct vectors nothing
 * boots, so this is the piece that makes an image actually runnable.
 *
 * The PRG is placed so that it occupies the top of the CPU address space; the
 * last 6 bytes of the final 16 KiB bank are the vector table. We let the user
 * name labels/addresses for the three vectors; any not given default to the
 * PRG load origin (RESET) and a safe RTI stub location is NOT invented — unset
 * NMI/IRQ default to the RESET address so the CPU never jumps into garbage.
 *
 * Usage:
 *   rom_link <out.nes> --prg <prg.bin> [--chr <chr.bin>]
 *            [--mapper N] [--battery] [--prg-banks N] [--chr-banks N]
 *            [--org HEX] [--reset HEX] [--nmi HEX] [--irq HEX]
 *
 *   --org    : CPU address the PRG is loaded at (default $8000 for 1 bank,
 *              $C000 for a 16 KiB image mirrored, etc.). Default: computed.
 *   --reset  : RESET vector (default = --org).
 *   --nmi    : NMI vector  (default = --reset).
 *   --irq    : IRQ/BRK vector (default = --reset).
 * ========================================================================== */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRG_BANK 16384u
#define CHR_BANK  8192u

static long file_read(const char *path, uint8_t **buf) {
    FILE *fp = fopen(path, "rb");
    if (!fp) { perror(path); return -1; }
    fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
    if (n < 0) { fclose(fp); return -1; }
    *buf = malloc((size_t)n ? (size_t)n : 1);
    if (fread(*buf, 1, (size_t)n, fp) != (size_t)n) { fclose(fp); free(*buf); return -1; }
    fclose(fp);
    return n;
}

static unsigned parse_hex(const char *s) { return (unsigned)strtoul(s, NULL, 16); }

int main(int argc, char **argv) {
    const char *out = NULL, *prg_path = NULL, *chr_path = NULL;
    int mapper = 0, battery = 0;
    int prg_banks = 0, chr_banks = 0;         /* 0 = auto from blob size */
    long org = -1, reset = -1, nmi = -1, irq = -1;

    for (int i = 1; i < argc; ++i) {
        if (!out && argv[i][0] != '-') { out = argv[i]; continue; }
        else if (!strcmp(argv[i], "--prg") && i+1 < argc) prg_path = argv[++i];
        else if (!strcmp(argv[i], "--chr") && i+1 < argc) chr_path = argv[++i];
        else if (!strcmp(argv[i], "--mapper") && i+1 < argc) mapper = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--battery")) battery = 1;
        else if (!strcmp(argv[i], "--prg-banks") && i+1 < argc) prg_banks = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--chr-banks") && i+1 < argc) chr_banks = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--org") && i+1 < argc) org = parse_hex(argv[++i]);
        else if (!strcmp(argv[i], "--reset") && i+1 < argc) reset = parse_hex(argv[++i]);
        else if (!strcmp(argv[i], "--nmi") && i+1 < argc) nmi = parse_hex(argv[++i]);
        else if (!strcmp(argv[i], "--irq") && i+1 < argc) irq = parse_hex(argv[++i]);
    }
    if (!out || !prg_path) {
        fprintf(stderr, "usage: rom_link <out.nes> --prg <prg.bin> [--chr <chr.bin>]\n"
                        "       [--mapper N] [--battery] [--prg-banks N] [--chr-banks N]\n"
                        "       [--org HEX] [--reset HEX] [--nmi HEX] [--irq HEX]\n");
        return 2;
    }

    uint8_t *prg = NULL, *chr = NULL;
    long prg_len = file_read(prg_path, &prg);
    if (prg_len < 0) return 1;
    long chr_len = 0;
    if (chr_path) { chr_len = file_read(chr_path, &chr); if (chr_len < 0) { free(prg); return 1; } }

    if (prg_banks == 0) prg_banks = (int)((prg_len + PRG_BANK - 1) / PRG_BANK);
    if (prg_banks == 0) prg_banks = 1;
    if (chr_banks == 0 && chr_len > 0) chr_banks = (int)((chr_len + CHR_BANK - 1) / CHR_BANK);

    unsigned prg_bytes = (unsigned)prg_banks * PRG_BANK;
    unsigned chr_bytes = (unsigned)chr_banks * CHR_BANK;

    if ((unsigned)prg_len > prg_bytes) {
        fprintf(stderr, "error: PRG blob (%ld B) exceeds %u banks (%u B)\n",
                prg_len, prg_banks, prg_bytes);
        free(prg); free(chr); return 1;
    }

    /* The CPU maps the final 16 KiB bank to end at $FFFF. Default org puts the
     * PRG at the start of its window. For an N-bank image the window base is
     * $10000 - prg_bytes (clamped into the 16-bit space for the top bank). */
    unsigned window_base = (prg_bytes >= 0x8000u) ? (0x10000u - 0x8000u)
                                                   : (0x10000u - prg_bytes);
    if (org < 0) org = window_base;
    if (reset < 0) reset = org;
    if (nmi < 0) nmi = reset;      /* never leave a vector pointing at garbage */
    if (irq < 0) irq = reset;

    /* Build the PRG image padded to prg_bytes, then stamp the vector table. */
    uint8_t *prg_img = calloc(prg_bytes, 1);
    memcpy(prg_img, prg, (size_t)prg_len);

    /* vector table lives in the last 6 bytes of the PRG image */
    unsigned vt = prg_bytes - 6;
    prg_img[vt + 0] = (uint8_t)(nmi   & 0xFF);
    prg_img[vt + 1] = (uint8_t)((nmi   >> 8) & 0xFF);
    prg_img[vt + 2] = (uint8_t)(reset & 0xFF);
    prg_img[vt + 3] = (uint8_t)((reset >> 8) & 0xFF);
    prg_img[vt + 4] = (uint8_t)(irq   & 0xFF);
    prg_img[vt + 5] = (uint8_t)((irq   >> 8) & 0xFF);

    /* 16-byte iNES header (NES 2.0 identifier left off => plain iNES; small). */
    uint8_t h[16]; memset(h, 0, sizeof(h));
    h[0]='N'; h[1]='E'; h[2]='S'; h[3]=0x1A;
    h[4]=(uint8_t)prg_banks;
    h[5]=(uint8_t)chr_banks;
    h[6]=(uint8_t)(((mapper & 0x0F) << 4) | (battery ? 0x02 : 0));
    h[7]=(uint8_t)(mapper & 0xF0);

    FILE *of = fopen(out, "wb");
    if (!of) { perror(out); free(prg); free(chr); free(prg_img); return 1; }
    fwrite(h, 1, 16, of);
    fwrite(prg_img, 1, prg_bytes, of);
    if (chr_bytes) {
        uint8_t *chr_img = calloc(chr_bytes, 1);
        if (chr_len) memcpy(chr_img, chr, (size_t)chr_len);
        fwrite(chr_img, 1, chr_bytes, of);
        free(chr_img);
    }
    fclose(of);

    printf("linked %s: %d PRG bank(s) (%u B), %d CHR bank(s) (%u B), mapper %d%s\n",
           out, prg_banks, prg_bytes, chr_banks, chr_bytes, mapper,
           battery ? ", battery" : "");
    printf("  vectors: NMI=$%04lX RESET=$%04lX IRQ=$%04lX  (org $%04lX)\n",
           nmi, reset, irq, org);

    free(prg); free(chr); free(prg_img);
    return 0;
}
