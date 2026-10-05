# Our own NES build tools. No third-party dependencies — just a C compiler.
CC     ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic

BINS = asm6502/asm6502 chr-encoder/chr_encode audio-builder/audio_build exec-harness/exec6502

all: $(BINS)

asm6502/asm6502: asm6502/asm6502.c
	$(CC) $(CFLAGS) -o $@ $<

chr-encoder/chr_encode: chr-encoder/chr_encode.c
	$(CC) $(CFLAGS) -o $@ $<

audio-builder/audio_build: audio-builder/audio_build.c
	$(CC) $(CFLAGS) -o $@ $<

exec-harness/exec6502: exec-harness/exec6502.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINS)

.PHONY: all clean
