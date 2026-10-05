# tools — our own NES build tools (no third-party dependencies)

Original replacements for the external tool *categories* a NES edition needs.
Every program here is **our own code**, written against the public NES/6502
facts. Nothing is derived from any third-party assembler, editor, tracker, or
emulator — the whole point is zero external dependencies.

| Tool | Replaces (category) | What it does |
|---|---|---|
| **`asm6502/`** | a 6502 assembler | Two-pass assembler: our small 6502 syntax → machine-code bytes. |
| **`chr-encoder/`** | a tile/CHR editor's export | Packs our 8×8 tile text (indices 0–3) into NES two-bit-plane CHR. |
| **`audio-builder/`** | a music tracker's export | Compiles our text score into our own `NTSND v1` byte stream. |
| **`exec-harness/`** | an emulator (for validation) | Minimal, deterministic 6502 stepper for golden-state assertions. |

## Build

```sh
cd tools && make          # builds all four, needs only a C compiler
```

## End-to-end example (verified)

```sh
# 1. assemble an original program
./asm6502/asm6502 examples/sum.s examples/sum.bin

# 2. run it in our own harness and assert on state
./exec-harness/exec6502 examples/sum.bin 8000 1000 --dump 2
#    -> A=$05  (2+3), zero page: 05 06   (our expected golden state)

# 3. encode an original 8x8 tile to CHR (16 bytes)
./chr-encoder/chr_encode examples/tile.txt examples/tile.chr

# 4. compile an original score to our audio byte stream
./audio-builder/audio_build examples/score.txt examples/song.snd
```

## Tool details

### asm6502
- Syntax: labels (`name:`), `.org`, `.byte`, `.word`, `;` comments.
- Addressing: implied, `#imm`, zp(,X/Y), abs(,X/Y), `(ind,X)`, `(ind),Y`,
  `(ind)`, relative branches, accumulator.
- A hand-authored subset of the 6502 ISA; extend the `ISA[]` table to grow it.

### chr-encoder
- Input: 8 lines of 8 digits (`0`–`3`) per tile; blank lines/`#` comments OK.
- Output: 16 bytes/tile — plane 0 (low bits) then plane 1 (high bits), per row.

### audio-builder
- Input lines: `<channel 0-3> <note|R> <frames 1-255>`; notes are name+octave
  (e.g. `C4`, `G#3`), `R` = rest.
- Output: `NTSND v1` — `53 01`, then 3-byte events `[chan][semitone][frames]`,
  terminated by `FF`. Designed for a tiny 6502 playback engine we write.

### exec-harness
- Loads a raw binary at a chosen address, runs up to N steps or until `BRK`.
- Prints final `A/X/Y/SP/P/PC`, flags, and optionally the first N zero-page
  bytes — so tests assert against **our** expected state, not an outside core.
- A deliberately small instruction subset for validation; not a full emulator.

## Scope & relationship to the rest of the repo

These feed the pipeline in [`../docs/EDITIONS.md`](../docs/EDITIONS.md):
author → build (asm6502 / chr-encoder / audio-builder) → validate (exec-harness
+ `../nts/` structural tools). They operate on **our own** source and emit
**our own** bytes; they never read, extract, or reproduce another game's
content.
