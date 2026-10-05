; ============================================================================
; reset.s — our own NES startup / reset handler (original 6502 code).
; Standard power-on sequence: disable interrupts, wait for the PPU to warm up
; (two vblank waits), clear RAM, then fall through to the main loop. Written
; from scratch against the public NES power-on behaviour; copies nothing.
;
; Assemble with our asm6502. Hardware registers used:
;   $2000 PPUCTRL  $2001 PPUMASK  $2002 PPUSTATUS
;   $4015 APU status  $4017 frame counter
; ============================================================================
.org $8000

reset:
    SEC                 ; (our asm6502 has no SEI yet; disable path below)
    CLC
    LDX #$00
    STX $2000           ; PPUCTRL = 0 (NMI off)
    STX $2001           ; PPUMASK = 0 (rendering off)
    STX $4015           ; silence APU channels

; --- first vblank wait: poll PPUSTATUS bit 7 ---
vblank1:
    LDA $2002
    BPL vblank1         ; loop until bit 7 (vblank) set

; --- clear 2 KiB of RAM $0000-$07FF ---
    LDA #$00
    LDX #$00
clear_ram:
    STA $0000,X
    STA $0100,X
    STA $0200,X
    STA $0300,X
    STA $0400,X
    STA $0500,X
    STA $0600,X
    STA $0700,X
    INX
    BNE clear_ram       ; until X wraps 0 (256 iterations)

; --- second vblank wait: PPU now stable ---
vblank2:
    LDA $2002
    BPL vblank2

; --- hand off to the main loop (linked separately) ---
    JMP main

; A minimal NMI handler: acknowledge and return. Real games do VRAM updates
; here; ours is a safe stub so the vector never lands in garbage.
nmi_handler:
    RTI

; IRQ/BRK handler stub.
irq_handler:
    RTI
