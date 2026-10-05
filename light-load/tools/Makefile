# Our own NES build tools. No third-party dependencies — just a C compiler.
CC     ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic

# Our own NES build tools. No third-party dependencies — a C and C++ compiler.
CXX    ?= c++
CXXFLAGS?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

BINS = asm6502/asm6502 chr-encoder/chr_encode audio-builder/audio_build \
       exec-harness/exec6502 rom-linker/rom_link \
       nametable-builder/nt_build palette-tool/pal_build \
       disasm/disasm6502 rom-splitter/rom_split \
       reorganize/reorganize finalize/finalize

all: $(BINS)

disasm/disasm6502: disasm/disasm6502.c
	$(CC) $(CFLAGS) -o $@ $<

rom-splitter/rom_split: rom-splitter/rom_split.c
	$(CC) $(CFLAGS) -o $@ $<

reorganize/reorganize: reorganize/reorganize.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

finalize/finalize: finalize/finalize.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

asm6502/asm6502: asm6502/asm6502.c
	$(CC) $(CFLAGS) -o $@ $<

chr-encoder/chr_encode: chr-encoder/chr_encode.c
	$(CC) $(CFLAGS) -o $@ $<

audio-builder/audio_build: audio-builder/audio_build.c
	$(CC) $(CFLAGS) -o $@ $<

exec-harness/exec6502: exec-harness/exec6502.c
	$(CC) $(CFLAGS) -o $@ $<

rom-linker/rom_link: rom-linker/rom_link.c
	$(CC) $(CFLAGS) -o $@ $<

nametable-builder/nt_build: nametable-builder/nt_build.c
	$(CC) $(CFLAGS) -o $@ $<

palette-tool/pal_build: palette-tool/pal_build.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINS)

.PHONY: all clean
