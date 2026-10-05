; ============================================================================
; audio.s — our own 6502 playback engine for the NTSND v1 stream.
; Reads the byte stream produced by tools/audio-builder and writes APU
; registers one event at a time, holding each for its frame count. Written
; from scratch against the public APU register interface; it is the 6502 half
; of our audio format (the builder is the host half).
;
; APU registers used (pulse 1): $4000-$4003; status $4015.
; Stream (NTSND v1): 'S' 0x01, then 3-byte events [chan][semitone][frames],
; terminated by $FF.
;
; Zero page: $30/$31 = stream pointer (lo/hi), $32 = frames left in event.
; ============================================================================
.org $9200

audio_init:
    LDA #$0F
    STA $4015           ; enable pulse1/pulse2/triangle/noise
    LDA #$00
    STA $32             ; no event loaded yet
    RTS

; advance by one frame; call once per NMI/frame.
audio_tick:
    LDA $32
    BEQ audio_next      ; current event finished -> load next
    DEC $32             ; still holding this note
    RTS

audio_next:
    LDY #$00
    LDA ($30),Y         ; channel byte (or $FF terminator)
    CMP #$FF
    BEQ audio_done
    ; we only drive pulse1 in this minimal engine; channel byte consumed
    LDA ($30),Y         ; re-read not needed; kept simple
    ; semitone -> we store it low in the timer for a simple pitch
    LDY #$01
    LDA ($30),Y
    STA $4002           ; pulse1 timer low (crude pitch from semitone)
    LDA #$08
    STA $4003           ; timer high + length counter load
    LDA #$BF
    STA $4000           ; duty + constant volume
    LDY #$02
    LDA ($30),Y
    STA $32             ; frames to hold
    ; advance stream pointer by 3 bytes
    CLC
    LDA $30
    ADC #$03
    STA $30
    LDA $31
    ADC #$00
    STA $31
    RTS

audio_done:
    LDA #$00
    STA $4000           ; silence pulse1
    RTS
