; ============================================================================
; input.s — our own controller reader (original 6502 code).
; Strobes the controller ports and shifts 8 button bits into a zero-page byte.
; Written from scratch against the public controller interface.
;
; Registers: $4016 (controller 1 strobe/read), $4017 (controller 2 read)
; Output: $20 = player 1 button state, bit order (MSB..LSB):
;         A B Select Start Up Down Left Right
; ============================================================================
.org $9100

read_pad1:
    LDA #$01
    STA $4016           ; strobe high: latch current button state
    LDA #$00
    STA $4016           ; strobe low: begin serial shift-out
    LDX #$08            ; 8 buttons
    LDA #$00
    STA $20             ; clear result
pad_loop:
    LDA $4016           ; read one button bit (bit 0)
    LSR A               ; shift its bit 0 into carry
    ROL $20             ; carry -> LSB of result, result shifts left
    DEX
    BNE pad_loop
    RTS

; test a button: load mask into A, AND with $20, Z flag set => not pressed
; masks:  A=$80 B=$40 Select=$20 Start=$10 Up=$08 Down=$04 Left=$02 Right=$01
check_button:
    AND $20
    RTS
