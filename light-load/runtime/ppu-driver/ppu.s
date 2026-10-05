; ============================================================================
; ppu.s — our own PPU driver (original 6502 code).
; Routines to load a palette, upload a nametable block, and run OAM sprite DMA.
; Written from scratch against the public PPU register interface.
;
; Registers: $2000 PPUCTRL  $2001 PPUMASK  $2003 OAMADDR  $2006 PPUADDR
;            $2007 PPUDATA  $4014 OAMDMA
; Convention: callers set up pointers in zero page before JSR.
;   $10/$11 = source address (lo/hi) for uploads
;   $12     = byte count for small uploads
; ============================================================================
.org $9000

; ---- load the 32-byte palette at PPU $3F00 ----
; expects $10/$11 -> 32 palette bytes in our data
ppu_load_palette:
    LDA #$3F
    STA $2006           ; PPUADDR high = $3F
    LDA #$00
    STA $2006           ; PPUADDR low  = $00  (=> $3F00)
    LDY #$00
pal_loop:
    LDA ($10),Y
    STA $2007
    INY
    CPY #$20            ; 32 bytes
    BNE pal_loop
    RTS

; ---- upload $12 bytes from ($10) to PPU address in A:X (hi:lo) ----
; call with A=hi target, X=lo target, $10/$11=src, $12=count
ppu_upload:
    STA $2006           ; high byte of PPU target
    STX $2006           ; low byte
    LDY #$00
up_loop:
    LDA ($10),Y
    STA $2007
    INY
    CPY $12
    BNE up_loop
    RTS

; ---- sprite DMA: copy page $0200 to OAM via $4014 ----
ppu_oam_dma:
    LDA #$00
    STA $2003           ; OAMADDR = 0
    LDA #$02
    STA $4014           ; trigger DMA from $0200
    RTS

; ---- enable background + sprite rendering ----
ppu_enable:
    LDA #$1E            ; PPUMASK: show bg + sprites, no clipping
    STA $2001
    LDA #$80            ; PPUCTRL: enable NMI
    STA $2000
    RTS
