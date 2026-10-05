/* ============================================================================
 * disasm6502 — "debuild": disassemble raw 6502 bytes back into readable source.
 * ----------------------------------------------------------------------------
 * Our own code. The inverse of tools/asm6502: it decodes a PRG blob (our own
 * build output) into 6502 mnemonics so we can inspect what we built. It is for
 * studying OUR OWN artifacts; it is not a tool for reproducing another game's
 * content, and it reads only bytes we hand it.
 *
 * Covers the same instruction subset asm6502 emits, plus common opcodes, so a
 * round trip (asm6502 -> disasm6502) is legible. Unknown bytes are shown as
 * ".byte $xx" so output always re-assembles conceptually.
 *
 * Usage:
 *   disasm6502 <in.bin> [load_hex]      (load address defaults to $8000)
 * ========================================================================== */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* addressing-mode kinds for formatting + length */
enum { IMP, IMM, ZP, ZPX, ZPY, ABS, ABX, ABY, INX, INY, IND, REL, ACC, UNK };

typedef struct { const char *mn; int mode; } Op;

/* 256-entry opcode table (subset filled; rest UNK -> .byte). */
static Op OPS[256];

static void set(int code, const char *mn, int mode) { OPS[code].mn = mn; OPS[code].mode = mode; }

static void init_table(void) {
    for (int i = 0; i < 256; ++i) { OPS[i].mn = NULL; OPS[i].mode = UNK; }
    /* loads */
    set(0xA9,"LDA",IMM); set(0xA5,"LDA",ZP); set(0xB5,"LDA",ZPX); set(0xAD,"LDA",ABS);
    set(0xBD,"LDA",ABX); set(0xB9,"LDA",ABY); set(0xA1,"LDA",INX); set(0xB1,"LDA",INY);
    set(0xA2,"LDX",IMM); set(0xA6,"LDX",ZP); set(0xB6,"LDX",ZPY); set(0xAE,"LDX",ABS); set(0xBE,"LDX",ABY);
    set(0xA0,"LDY",IMM); set(0xA4,"LDY",ZP); set(0xB4,"LDY",ZPX); set(0xAC,"LDY",ABS); set(0xBC,"LDY",ABX);
    /* stores */
    set(0x85,"STA",ZP); set(0x95,"STA",ZPX); set(0x8D,"STA",ABS); set(0x9D,"STA",ABX);
    set(0x99,"STA",ABY); set(0x81,"STA",INX); set(0x91,"STA",INY);
    set(0x86,"STX",ZP); set(0x96,"STX",ZPY); set(0x8E,"STX",ABS);
    set(0x84,"STY",ZP); set(0x94,"STY",ZPX); set(0x8C,"STY",ABS);
    /* transfers / stack */
    set(0xAA,"TAX",IMP); set(0xA8,"TAY",IMP); set(0x8A,"TXA",IMP); set(0x98,"TYA",IMP);
    set(0x9A,"TXS",IMP); set(0xBA,"TSX",IMP);
    set(0x48,"PHA",IMP); set(0x68,"PLA",IMP); set(0x08,"PHP",IMP); set(0x28,"PLP",IMP);
    /* inc/dec */
    set(0xE8,"INX",IMP); set(0xC8,"INY",IMP); set(0xCA,"DEX",IMP); set(0x88,"DEY",IMP);
    set(0xE6,"INC",ZP); set(0xF6,"INC",ZPX); set(0xEE,"INC",ABS); set(0xFE,"INC",ABX);
    set(0xC6,"DEC",ZP); set(0xD6,"DEC",ZPX); set(0xCE,"DEC",ABS); set(0xDE,"DEC",ABX);
    /* arithmetic / logic */
    set(0x69,"ADC",IMM); set(0x65,"ADC",ZP); set(0x6D,"ADC",ABS);
    set(0xE9,"SBC",IMM); set(0xE5,"SBC",ZP); set(0xED,"SBC",ABS);
    set(0x29,"AND",IMM); set(0x25,"AND",ZP); set(0x2D,"AND",ABS);
    set(0x09,"ORA",IMM); set(0x05,"ORA",ZP); set(0x0D,"ORA",ABS);
    set(0x49,"EOR",IMM); set(0x45,"EOR",ZP);
    set(0xC9,"CMP",IMM); set(0xC5,"CMP",ZP); set(0xCD,"CMP",ABS);
    set(0xE0,"CPX",IMM); set(0xE4,"CPX",ZP); set(0xC0,"CPY",IMM); set(0xC4,"CPY",ZP);
    /* shifts */
    set(0x0A,"ASL",ACC); set(0x06,"ASL",ZP); set(0x4A,"LSR",ACC); set(0x46,"LSR",ZP);
    set(0x2A,"ROL",ACC); set(0x6A,"ROR",ACC);
    /* jumps / branches */
    set(0x4C,"JMP",ABS); set(0x6C,"JMP",IND); set(0x20,"JSR",ABS);
    set(0x60,"RTS",IMP); set(0x40,"RTI",IMP);
    set(0xF0,"BEQ",REL); set(0xD0,"BNE",REL); set(0x90,"BCC",REL); set(0xB0,"BCS",REL);
    set(0x10,"BPL",REL); set(0x30,"BMI",REL); set(0x50,"BVC",REL); set(0x70,"BVS",REL);
    /* flags / misc */
    set(0x18,"CLC",IMP); set(0x38,"SEC",IMP); set(0x58,"CLI",IMP); set(0x78,"SEI",IMP);
    set(0xD8,"CLD",IMP); set(0xF8,"SED",IMP); set(0xB8,"CLV",IMP);
    set(0xEA,"NOP",IMP); set(0x00,"BRK",IMP);
}

static int mode_len(int mode) {
    switch (mode) {
        case IMP: case ACC: return 1;
        case IMM: case ZP: case ZPX: case ZPY: case INX: case INY: case REL: return 2;
        case ABS: case ABX: case ABY: case IND: return 3;
        default: return 1;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: disasm6502 <in.bin> [load_hex]\n"); return 2; }
    unsigned load = (argc >= 3) ? (unsigned)strtoul(argv[2], NULL, 16) : 0x8000;

    FILE *fp = fopen(argv[1], "rb"); if (!fp) { perror("open"); return 1; }
    fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
    if (n <= 0) { fclose(fp); fprintf(stderr, "empty input\n"); return 1; }
    uint8_t *b = malloc((size_t)n);
    if (fread(b, 1, (size_t)n, fp) != (size_t)n) { fclose(fp); free(b); return 1; }
    fclose(fp);

    init_table();
    printf("; disassembly of %s (%ld bytes, load $%04X)\n", argv[1], n, load);
    printf(".org $%04X\n", load);

    long i = 0;
    while (i < n) {
        uint8_t op = b[i];
        Op o = OPS[op];
        unsigned pc = load + (unsigned)i;
        if (!o.mn) { printf("    .byte $%02X\n", op); ++i; continue; }

        int len = mode_len(o.mode);
        if (i + len > n) { printf("    .byte $%02X\n", op); ++i; continue; }

        uint8_t lo = (len >= 2) ? b[i+1] : 0;
        uint8_t hi = (len == 3) ? b[i+2] : 0;
        unsigned abs = (unsigned)(lo | (hi << 8));

        switch (o.mode) {
            case IMP: printf("    %s\n", o.mn); break;
            case ACC: printf("    %s A\n", o.mn); break;
            case IMM: printf("    %s #$%02X\n", o.mn, lo); break;
            case ZP:  printf("    %s $%02X\n", o.mn, lo); break;
            case ZPX: printf("    %s $%02X,X\n", o.mn, lo); break;
            case ZPY: printf("    %s $%02X,Y\n", o.mn, lo); break;
            case ABS: printf("    %s $%04X\n", o.mn, abs); break;
            case ABX: printf("    %s $%04X,X\n", o.mn, abs); break;
            case ABY: printf("    %s $%04X,Y\n", o.mn, abs); break;
            case INX: printf("    %s ($%02X,X)\n", o.mn, lo); break;
            case INY: printf("    %s ($%02X),Y\n", o.mn, lo); break;
            case IND: printf("    %s ($%04X)\n", o.mn, abs); break;
            case REL: {
                int8_t d = (int8_t)lo;
                unsigned target = (unsigned)(pc + 2 + d);
                printf("    %s $%04X\n", o.mn, target);
            } break;
            default: printf("    .byte $%02X\n", op); break;
        }
        i += len;
    }
    free(b);
    return 0;
}
