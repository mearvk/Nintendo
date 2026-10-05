/* ============================================================================
 * exec6502 — original minimal 6502 execution harness.
 * ----------------------------------------------------------------------------
 * Our own code. A small, deterministic 6502 CPU stepper for VALIDATING our own
 * code in a 64 KB flat memory, with golden-state assertions. Not a full
 * emulator and not derived from any third-party core — it exists so our
 * verification series depends only on code we wrote.
 *
 * It loads a raw binary at a chosen address, runs up to N instructions (or
 * until BRK), and prints the final CPU/zero-page state so tests can assert on
 * known values.
 *
 * Usage:
 *   exec6502 <program.bin> <load_hex> [max_steps] [--dump N]
 *     load_hex   : e.g. 8000  (also sets PC)
 *     max_steps  : default 100000
 *     --dump N   : also print the first N zero-page bytes
 * ============================================================================ */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t mem[65536];

/* CPU state */
static uint16_t PC;
static uint8_t  A, Xr, Yr, SP, P;
/* P flags: N V - B D I Z C */
enum { C=0x01, Z=0x02, I=0x04, D=0x08, B=0x10, U=0x20, V=0x40, N=0x80 };

/* Minimal register models so runtime boot code can be validated.
 * We do NOT emulate real timing; we model just enough that the common
 * "poll PPUSTATUS vblank bit" warm-up pattern makes forward progress:
 *   $2002 (PPUSTATUS): bit 7 toggles toward "set" on repeated reads, so a
 *   BPL wait loop terminates deterministically instead of spinning forever.
 *   $4016 (controller): returns 0 (no buttons) each read.
 * These are validation stubs, not an emulator. */
static int ppustatus_reads = 0;

static uint8_t rd(uint16_t a) {
    if (a == 0x2002) {
        /* set the vblank bit after a couple of polls, then it stays set */
        return (uint8_t)((++ppustatus_reads >= 2) ? 0x80 : 0x00);
    }
    if (a == 0x4016 || a == 0x4017) return 0x00;
    return mem[a];
}
static void wr(uint16_t a, uint8_t v) {
    /* writes to registers are accepted and dropped (OAMDMA etc. are no-ops) */
    if (a >= 0x2000 && a <= 0x401F) { mem[a] = v; return; }
    mem[a] = v;
}
static uint8_t  fetch(void) { return rd(PC++); }
static uint16_t fetch16(void){ uint16_t lo=fetch(); uint16_t hi=fetch(); return (uint16_t)(lo|(hi<<8)); }

static void setzn(uint8_t v){ P = (uint8_t)((P & ~(Z|N)) | (v?0:Z) | (v&0x80)); }
static void push(uint8_t v){ wr(0x100 + SP--, v); }
static uint8_t pop(void){ return rd(0x100 + ++SP); }

/* addressing helpers */
static uint16_t a_abs(void){ return fetch16(); }
static uint16_t a_abx(void){ return (uint16_t)(fetch16()+Xr); }
static uint16_t a_aby(void){ return (uint16_t)(fetch16()+Yr); }
static uint16_t a_zp (void){ return fetch(); }
static uint16_t a_zpx(void){ return (uint8_t)(fetch()+Xr); }
static uint16_t a_zpy(void){ return (uint8_t)(fetch()+Yr); }
static uint16_t a_inx(void){ uint8_t z=(uint8_t)(fetch()+Xr); return (uint16_t)(rd(z)|(rd((uint8_t)(z+1))<<8)); }
static uint16_t a_iny(void){ uint8_t z=fetch(); uint16_t b=(uint16_t)(rd(z)|(rd((uint8_t)(z+1))<<8)); return (uint16_t)(b+Yr); }

static void adc(uint8_t m){
    unsigned s = A + m + (P&C?1:0);
    P &= ~(C|V);
    if (s>0xFF) P|=C;
    if (~(A^m)&(A^s)&0x80) P|=V;
    A=(uint8_t)s; setzn(A);
}
static void sbc(uint8_t m){ adc((uint8_t)~m); }
static void cmp(uint8_t r, uint8_t m){ unsigned t=r-m; P&=~(C|Z|N); if(r>=m)P|=C; setzn((uint8_t)t); if(((uint8_t)t)&0) {} }

/* Execute up to max steps. Returns steps run. */
static long run(long max){
    long steps=0;
    for (; steps<max; ++steps){
        uint8_t op = fetch();
        switch(op){
        /* LDA */
        case 0xA9: A=fetch(); setzn(A); break;
        case 0xA5: A=rd(a_zp()); setzn(A); break;
        case 0xB5: A=rd(a_zpx()); setzn(A); break;
        case 0xAD: A=rd(a_abs()); setzn(A); break;
        case 0xBD: A=rd(a_abx()); setzn(A); break;
        case 0xB9: A=rd(a_aby()); setzn(A); break;
        case 0xA1: A=rd(a_inx()); setzn(A); break;
        case 0xB1: A=rd(a_iny()); setzn(A); break;
        /* LDX / LDY */
        case 0xA2: Xr=fetch(); setzn(Xr); break;
        case 0xA6: Xr=rd(a_zp()); setzn(Xr); break;
        case 0xAE: Xr=rd(a_abs()); setzn(Xr); break;
        case 0xA0: Yr=fetch(); setzn(Yr); break;
        case 0xA4: Yr=rd(a_zp()); setzn(Yr); break;
        case 0xAC: Yr=rd(a_abs()); setzn(Yr); break;
        /* STA / STX / STY */
        case 0x85: wr(a_zp(),A); break;
        case 0x95: wr(a_zpx(),A); break;
        case 0x8D: wr(a_abs(),A); break;
        case 0x9D: wr(a_abx(),A); break;
        case 0x99: wr(a_aby(),A); break;
        case 0x81: wr(a_inx(),A); break;
        case 0x91: wr(a_iny(),A); break;
        case 0x86: wr(a_zp(),Xr); break;
        case 0x96: wr(a_zpy(),Xr); break;
        case 0x8E: wr(a_abs(),Xr); break;
        case 0x84: wr(a_zp(),Yr); break;
        case 0x94: wr(a_zpx(),Yr); break;
        case 0x8C: wr(a_abs(),Yr); break;
        /* transfers */
        case 0xAA: Xr=A; setzn(Xr); break;
        case 0xA8: Yr=A; setzn(Yr); break;
        case 0x8A: A=Xr; setzn(A); break;
        case 0x98: A=Yr; setzn(A); break;
        /* inc/dec */
        case 0xE8: Xr++; setzn(Xr); break;
        case 0xC8: Yr++; setzn(Yr); break;
        case 0xCA: Xr--; setzn(Xr); break;
        case 0x88: Yr--; setzn(Yr); break;
        case 0xE6:{uint16_t a=a_zp(); uint8_t v=(uint8_t)(rd(a)+1); wr(a,v); setzn(v);}break;
        case 0xEE:{uint16_t a=a_abs();uint8_t v=(uint8_t)(rd(a)+1); wr(a,v); setzn(v);}break;
        case 0xC6:{uint16_t a=a_zp(); uint8_t v=(uint8_t)(rd(a)-1); wr(a,v); setzn(v);}break;
        case 0xCE:{uint16_t a=a_abs();uint8_t v=(uint8_t)(rd(a)-1); wr(a,v); setzn(v);}break;
        /* arithmetic/logic (immediate + zp + abs subset) */
        case 0x69: adc(fetch()); break;
        case 0x65: adc(rd(a_zp())); break;
        case 0x6D: adc(rd(a_abs())); break;
        case 0xE9: sbc(fetch()); break;
        case 0xE5: sbc(rd(a_zp())); break;
        case 0x29: A&=fetch(); setzn(A); break;
        case 0x25: A&=rd(a_zp()); setzn(A); break;
        case 0x09: A|=fetch(); setzn(A); break;
        case 0x05: A|=rd(a_zp()); setzn(A); break;
        case 0x49: A^=fetch(); setzn(A); break;
        case 0xC9: cmp(A,fetch()); break;
        case 0xC5: cmp(A,rd(a_zp())); break;
        case 0xE0: cmp(Xr,fetch()); break;
        case 0xC0: cmp(Yr,fetch()); break;
        /* shifts (accumulator) */
        case 0x0A: P=(uint8_t)((P&~C)|((A&0x80)?C:0)); A=(uint8_t)(A<<1); setzn(A); break;
        case 0x4A: P=(uint8_t)((P&~C)|((A&0x01)?C:0)); A=(uint8_t)(A>>1); setzn(A); break;
        /* jumps/branches */
        case 0x4C: PC=a_abs(); break;
        case 0x6C:{uint16_t p=a_abs(); PC=(uint16_t)(rd(p)|(rd((uint16_t)(p+1))<<8));}break;
        case 0x20:{uint16_t t=a_abs(); uint16_t r=(uint16_t)(PC-1); push((uint8_t)(r>>8)); push((uint8_t)r); PC=t;}break;
        case 0x60:{uint8_t lo=pop(); uint8_t hi=pop(); PC=(uint16_t)((lo|(hi<<8))+1);}break;
        case 0xF0:{int8_t d=(int8_t)fetch(); if(P&Z)PC=(uint16_t)(PC+d);}break;
        case 0xD0:{int8_t d=(int8_t)fetch(); if(!(P&Z))PC=(uint16_t)(PC+d);}break;
        case 0x90:{int8_t d=(int8_t)fetch(); if(!(P&C))PC=(uint16_t)(PC+d);}break;
        case 0xB0:{int8_t d=(int8_t)fetch(); if(P&C)PC=(uint16_t)(PC+d);}break;
        case 0x10:{int8_t d=(int8_t)fetch(); if(!(P&N))PC=(uint16_t)(PC+d);}break;
        case 0x30:{int8_t d=(int8_t)fetch(); if(P&N)PC=(uint16_t)(PC+d);}break;
        /* flags/misc */
        case 0x18: P&=~C; break;
        case 0x38: P|=C; break;
        case 0xEA: break;
        case 0x00: /* BRK: stop the harness */ return steps+1;
        default:
            fprintf(stderr, "unimplemented opcode $%02X at $%04X\n", op, (uint16_t)(PC-1));
            return steps;
        }
    }
    return steps;
}

int main(int argc, char **argv){
    if (argc < 3){ fprintf(stderr,"usage: exec6502 <program.bin> <load_hex> [max_steps] [--dump N]\n"); return 2; }
    uint16_t load=(uint16_t)strtol(argv[2],NULL,16);
    long max = (argc>=4 && strncmp(argv[3],"--",2)!=0) ? atol(argv[3]) : 100000;
    int dumpn = 0;
    for (int i=3;i<argc;++i) if(!strcmp(argv[i],"--dump") && i+1<argc) dumpn=atoi(argv[i+1]);

    FILE *fp=fopen(argv[1],"rb"); if(!fp){perror("open");return 1;}
    size_t n=fread(mem+load,1,(size_t)(65536-load),fp); fclose(fp);

    PC=load; A=Xr=Yr=0; SP=0xFD; P=U|I;
    long steps=run(max);

    printf("ran %ld steps; loaded %zu bytes at $%04X\n", steps, n, load);
    printf("A=$%02X X=$%02X Y=$%02X SP=$%02X P=$%02X PC=$%04X\n",
           A,Xr,Yr,SP,P,PC);
    printf("flags: N%d V%d D%d I%d Z%d C%d\n",
           !!(P&N),!!(P&V),!!(P&D),!!(P&I),!!(P&Z),!!(P&C));
    if (dumpn>0){
        printf("zero page [0..%d]:", dumpn-1);
        for (int i=0;i<dumpn && i<256;++i){ if(i%16==0)printf("\n  "); printf("%02X ",mem[i]); }
        printf("\n");
    }
    return 0;
}
