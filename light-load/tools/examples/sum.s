; sum.s — original demo: add 2 + 3, store result at $00, then stop.
; Verifies our assembler + execution harness end to end.
.org $8000
start:
    LDA #$02        ; A = 2
    CLC
    ADC #$03        ; A = 2 + 3 = 5
    STA $00         ; zero page $00 = 5
    LDX #$05
    INX             ; X = 6
    STX $01         ; zero page $01 = 6
    BRK             ; stop the harness
