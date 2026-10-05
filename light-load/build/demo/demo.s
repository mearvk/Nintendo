; demo.s — a self-contained original boot program for the build driver.
; Reset handler + main loop in one PRG blob (NROM, loads at $C000).
; Clears RAM, sets a marker, and spins — enough to prove a complete,
; bootable image produced entirely by our own toolchain.
.org $C000

reset:
    LDX #$00
    STX $2000           ; PPUCTRL = 0 (NMI off)
    STX $2001           ; PPUMASK = 0 (rendering off)
    STX $4015           ; silence APU

vwait:
    LDA $2002           ; poll PPUSTATUS
    BPL vwait           ; wait for vblank

    LDA #$00            ; clear zero page
    LDX #$00
clrzp:
    STA $0000,X
    INX
    BNE clrzp

    LDA #$5A            ; marker so the harness can assert boot ran
    STA $00

main:
    JMP main            ; spin forever (a real game runs its loop here)
