/* ============================================================================
 * asm6502 — a minimal, original two-pass 6502 assembler.
 * ----------------------------------------------------------------------------
 * Our own code. Translates a small, well-defined 6502 assembly syntax into raw
 * machine-code bytes. Not derived from any third-party assembler; it is the
 * "write our own tooling" replacement for external assemblers.
 *
 * Supported (deliberately small, extend as needed):
 *   - labels:            name:
 *   - directives:        .org <addr> , .byte b0,b1,... , .word w0,...
 *   - comments:          ; to end of line
 *   - addressing modes:  implied, #imm, zp, zp,X, zp,Y, abs, abs,X, abs,Y,
 *                        (ind,X), (ind),Y, (ind), relative (branches)
 *
 * Usage:
 *   asm6502 <in.s> <out.bin>
 * ========================================================================== */
#define _GNU_SOURCE
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strcasecmp / strncasecmp */

/* ---- opcode table: name + per-mode opcode (0xFF = unsupported) ---------- */
enum { M_IMP, M_IMM, M_ZP, M_ZPX, M_ZPY, M_ABS, M_ABX, M_ABY,
       M_INX, M_INY, M_IND, M_REL, M_ACC, N_MODES };

typedef struct { const char *mn; uint8_t op[N_MODES]; } Instr;

#define X 0xFF
/* A compact, hand-authored subset of the 6502 ISA. */
static const Instr ISA[] = {
/*          IMP  IMM  ZP   ZPX  ZPY  ABS  ABX  ABY  INX  INY  IND  REL  ACC */
{"LDA", { X,  0xA9,0xA5,0xB5, X,  0xAD,0xBD,0xB9,0xA1,0xB1, X,   X,   X }},
{"LDX", { X,  0xA2,0xA6, X,  0xB6,0xAE, X,  0xBE, X,   X,   X,   X,   X }},
{"LDY", { X,  0xA0,0xA4,0xB4, X,  0xAC,0xBC, X,   X,   X,   X,   X,   X }},
{"STA", { X,   X,  0x85,0x95, X,  0x8D,0x9D,0x99,0x81,0x91, X,   X,   X }},
{"STX", { X,   X,  0x86, X,  0x96,0x8E, X,   X,   X,   X,   X,   X,   X }},
{"STY", { X,   X,  0x84,0x94, X,  0x8C, X,   X,   X,   X,   X,   X,   X }},
{"TAX", {0xAA, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"TAY", {0xA8, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"TXA", {0x8A, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"TYA", {0x98, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"INX", {0xE8, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"INY", {0xC8, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"DEX", {0xCA, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"DEY", {0x88, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"INC", { X,   X,  0xE6,0xF6, X,  0xEE,0xFE, X,   X,   X,   X,   X,   X }},
{"DEC", { X,   X,  0xC6,0xD6, X,  0xCE,0xDE, X,   X,   X,   X,   X,   X }},
{"ADC", { X,  0x69,0x65,0x75, X,  0x6D,0x7D,0x79,0x61,0x71, X,   X,   X }},
{"SBC", { X,  0xE9,0xE5,0xF5, X,  0xED,0xFD,0xF9,0xE1,0xF1, X,   X,   X }},
{"AND", { X,  0x29,0x25,0x35, X,  0x2D,0x3D,0x39,0x21,0x31, X,   X,   X }},
{"ORA", { X,  0x09,0x05,0x15, X,  0x0D,0x1D,0x19,0x01,0x11, X,   X,   X }},
{"EOR", { X,  0x49,0x45,0x55, X,  0x4D,0x5D,0x59,0x41,0x51, X,   X,   X }},
{"CMP", { X,  0xC9,0xC5,0xD5, X,  0xCD,0xDD,0xD9,0xC1,0xD1, X,   X,   X }},
{"CPX", { X,  0xE0,0xE4, X,   X,  0xEC, X,   X,   X,   X,   X,   X,   X }},
{"CPY", { X,  0xC0,0xC4, X,   X,  0xCC, X,   X,   X,   X,   X,   X,   X }},
{"ASL", {0x0A, X,  0x06,0x16, X,  0x0E,0x1E, X,   X,   X,   X,   X,  0x0A}},
{"LSR", {0x4A, X,  0x46,0x56, X,  0x4E,0x5E, X,   X,   X,   X,   X,  0x4A}},
{"JMP", { X,   X,   X,   X,   X,  0x4C, X,   X,   X,   X,  0x6C, X,   X }},
{"JSR", { X,   X,   X,   X,   X,  0x20, X,   X,   X,   X,   X,   X,   X }},
{"RTS", {0x60, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"RTI", {0x40, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"BEQ", { X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,  0xF0, X }},
{"BNE", { X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,  0xD0, X }},
{"BCC", { X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,  0x90, X }},
{"BCS", { X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,  0xB0, X }},
{"BPL", { X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,  0x10, X }},
{"BMI", { X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,  0x30, X }},
{"CLC", {0x18, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"SEC", {0x38, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"NOP", {0xEA, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
{"BRK", {0x00, X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X,   X }},
};
#undef X
static const int ISA_N = (int)(sizeof(ISA) / sizeof(ISA[0]));

/* ---- label table -------------------------------------------------------- */
typedef struct { char name[48]; int addr; } Label;
static Label labels[1024];
static int   nlabels;

static int label_find(const char *n) {
    for (int i = 0; i < nlabels; ++i)
        if (strcmp(labels[i].name, n) == 0) return labels[i].addr;
    return -1;
}
static void label_add(const char *n, int addr) {
    if (nlabels < (int)(sizeof(labels)/sizeof(labels[0]))) {
        strncpy(labels[nlabels].name, n, sizeof(labels[0].name)-1);
        labels[nlabels].addr = addr;
        ++nlabels;
    }
}

/* ---- output buffer ------------------------------------------------------ */
static uint8_t out[1 << 20];
static int     outlen;
static int     org = 0x8000;   /* default PRG load address */

static void emit(uint8_t b) { if (outlen < (int)sizeof(out)) out[outlen++] = b; }

/* trim leading/trailing whitespace, strip comments */
static char *clean(char *s) {
    char *c = strchr(s, ';'); if (c) *c = 0;
    while (*s && isspace((unsigned char)*s)) ++s;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

static int parse_num(const char *s, int *ok) {
    int v; *ok = 1;
    if (s[0] == '$')      { v = (int)strtol(s + 1, NULL, 16); }
    else if (s[0] == '%') { v = (int)strtol(s + 1, NULL, 2);  }
    else if (isdigit((unsigned char)s[0]) || s[0]=='-') { v = atoi(s); }
    else {
        int la = label_find(s);
        if (la < 0) { *ok = 0; return 0; }
        v = la;
    }
    return v;
}

/* Determine mode + operand value from the operand text. */
static int decode_operand(const char *mn, char *operand, int pc,
                          int *mode, int *val) {
    char o[128];
    strncpy(o, operand, sizeof(o)-1); o[sizeof(o)-1]=0;
    int ok;

    if (o[0] == 0) { *mode = M_IMP; *val = 0; return 1; }
    if (strcasecmp(o, "A") == 0) { *mode = M_ACC; *val = 0; return 1; }

    if (o[0] == '#') { *mode = M_IMM; *val = parse_num(o+1, &ok) & 0xFF; return ok; }

    /* indirect forms */
    if (o[0] == '(') {
        char *comma = strstr(o, ",X)");
        if (comma) { *comma = 0; *mode = M_INX; *val = parse_num(o+1,&ok)&0xFF; return ok; }
        char *tail = strstr(o, "),Y");
        if (tail) { *tail = 0; *mode = M_INY; *val = parse_num(o+1,&ok)&0xFF; return ok; }
        char *end = strchr(o, ')');
        if (end) { *end = 0; *mode = M_IND; *val = parse_num(o+1,&ok)&0xFFFF; return ok; }
    }

    /* indexed */
    char *cx = strstr(o, ",X");
    char *cy = strstr(o, ",Y");
    if (cx) { *cx = 0; int v = parse_num(o,&ok); *val=v;
              *mode = (v>=0 && v<=0xFF) ? M_ZPX : M_ABX; return ok; }
    if (cy) { *cy = 0; int v = parse_num(o,&ok); *val=v;
              *mode = (v>=0 && v<=0xFF) ? M_ZPY : M_ABY; return ok; }

    /* branches are relative; detect by mnemonic starting with 'B' (not BIT/BRK) */
    if ((mn[0]=='B') && strcasecmp(mn,"BRK") && strcasecmp(mn,"BIT")) {
        int target = parse_num(o,&ok); *mode = M_REL;
        *val = (target - (pc + 2)) & 0xFF; return ok;
    }

    int v = parse_num(o,&ok); *val = v;
    *mode = (v>=0 && v<=0xFF) ? M_ZP : M_ABS;
    return ok;
}

static const Instr *find_instr(const char *mn) {
    for (int i = 0; i < ISA_N; ++i)
        if (strcasecmp(ISA[i].mn, mn) == 0) return &ISA[i];
    return NULL;
}

static int mode_len(int mode) {
    switch (mode) {
        case M_IMP: case M_ACC: return 1;
        case M_IMM: case M_ZP: case M_ZPX: case M_ZPY:
        case M_INX: case M_INY: case M_REL: return 2;
        default: return 3;
    }
}

/* Pass over the file; pass==1 collects labels, pass==2 emits bytes. */
static int assemble_pass(FILE *fp, int pass) {
    char line[256];
    int pc = org;
    rewind(fp);
    while (fgets(line, sizeof(line), fp)) {
        char *s = clean(line);
        if (!*s) continue;

        /* label definition */
        char *colon = strchr(s, ':');
        if (colon) {
            *colon = 0;
            if (pass == 1 && label_find(s) < 0) label_add(s, pc);
            s = clean(colon + 1);
            if (!*s) continue;
        }

        /* directives */
        if (s[0] == '.') {
            if (strncasecmp(s, ".org", 4) == 0) {
                int ok; org = parse_num(clean(s+4), &ok); pc = org;
                if (pass == 2) { /* no bytes */ }
                continue;
            }
            if (strncasecmp(s, ".byte", 5) == 0) {
                char *p = clean(s+5); char *tok = strtok(p, ",");
                while (tok) {
                    int ok; int v = parse_num(clean(tok), &ok);
                    if (pass == 2) emit((uint8_t)(v & 0xFF));
                    pc++;
                    tok = strtok(NULL, ",");
                }
                continue;
            }
            if (strncasecmp(s, ".word", 5) == 0) {
                char *p = clean(s+5); char *tok = strtok(p, ",");
                while (tok) { int ok; int v = parse_num(clean(tok), &ok);
                    if (pass == 2) { emit((uint8_t)(v & 0xFF)); emit((uint8_t)((v>>8)&0xFF)); }
                    pc += 2; tok = strtok(NULL, ","); }
                continue;
            }
            fprintf(stderr, "unknown directive: %s\n", s); return 1;
        }

        /* instruction: mnemonic [operand] */
        char mn[16] = {0}, operand[200] = {0};
        int n = sscanf(s, "%15s %199[^\n]", mn, operand);
        if (n < 1) continue;
        char *op = clean(operand);

        const Instr *ins = find_instr(mn);
        if (!ins) { fprintf(stderr, "unknown mnemonic: %s\n", mn); return 1; }

        int mode, val;
        if (!decode_operand(mn, op, pc, &mode, &val)) {
            if (pass == 2) { fprintf(stderr, "bad operand: %s %s\n", mn, op); return 1; }
            mode = M_ABS; val = 0; /* assume widest in pass 1 */
        }
        uint8_t opcode = ins->op[mode];
        if (opcode == 0xFF && pass == 2) {
            fprintf(stderr, "unsupported mode for %s\n", mn); return 1;
        }

        int len = mode_len(mode);
        if (pass == 2) {
            emit(opcode);
            if (len >= 2) emit((uint8_t)(val & 0xFF));
            if (len == 3) emit((uint8_t)((val >> 8) & 0xFF));
        }
        pc += len;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: asm6502 <in.s> <out.bin>\n"); return 2; }
    FILE *fp = fopen(argv[1], "r");
    if (!fp) { perror("open"); return 1; }

    if (assemble_pass(fp, 1)) { fclose(fp); return 1; }  /* collect labels */
    outlen = 0;
    if (assemble_pass(fp, 2)) { fclose(fp); return 1; }  /* emit bytes */
    fclose(fp);

    FILE *of = fopen(argv[2], "wb");
    if (!of) { perror("out"); return 1; }
    fwrite(out, 1, outlen, of);
    fclose(of);
    printf("assembled %d bytes -> %s (org $%04X, %d labels)\n",
           outlen, argv[2], org, nlabels);
    return 0;
}
