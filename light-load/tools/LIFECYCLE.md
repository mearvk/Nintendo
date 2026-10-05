# Lifecycle tools — debuild, build, reorganize, finalize

The project-lifecycle tools that sit alongside the authoring/validation set.
All our own code, zero third-party dependencies.

| Phase | Tool | Dir | What it does |
|---|---|---|---|
| **build** | `rom_link` | `rom-linker/` | Assemble a bootable `.nes` (header + PRG + CHR + interrupt vectors). |
| **build (one-shot)** | `build_driver` | `../build/` | Manifest-driven: assemble → encode → link → validate. |
| **debuild** | `disasm6502` | `disasm/` | Disassemble a PRG blob back into readable 6502 source (inverse of `asm6502`). |
| **debuild (unlink)** | `rom_split` | `rom-splitter/` | Split a `.nes` into `.hdr` + `.prg` + `.chr`; prints the vector table (inverse of `rom_link`). |
| **reorganize** | `reorganize` | `reorganize/` | Verify/normalize project layout; flag stray build artifacts. Report-only unless `--apply`. |
| **finalize** | `finalize` | `finalize/` | Release gate: structure + split + size must pass, then stamp a `RELEASE.txt` manifest. |

## Build

```sh
cd tools && make        # builds every tool (C + C++), needs only compilers
```

## Lifecycle walkthrough (verified)

```sh
# BUILD a bootable image from our own source
./asm6502/asm6502 demo.s demo.bin
./rom-linker/rom_link demo.nes --prg demo.bin --mapper 0 --org C000 --reset C000

# DEBUILD: inspect what we built
tail -c +17 demo.nes > demo.prg
./disasm/disasm6502 demo.prg C000        # -> LDA #$01 / STA $00 / ... / JMP $C000
./rom-splitter/rom_split demo.nes parts  # -> parts.hdr, parts.prg (+ vectors)

# REORGANIZE: check the workspace layout
./reorganize/reorganize ..               # reports missing dirs + stray artifacts

# FINALIZE: gate the release and stamp a manifest
./finalize/finalize demo.nes --nts ../nts/nts-analyze \
    --split ./rom-splitter/rom_split --max 100000 --out demo.RELEASE.txt
#   -> gates: structure, split, size all pass; status=FINALIZED
```

## Tool notes

### disasm6502 (debuild)
- Decodes the same instruction subset `asm6502` emits, plus common opcodes.
- Unknown bytes print as `.byte $xx`, so output is always legible and the
  padding tail reads as `BRK`.
- Operates on bytes we hand it — for inspecting **our own** artifacts.

### rom_split (unlink)
- Writes `<prefix>.hdr` (16 B), `<prefix>.prg`, and `<prefix>.chr` (if present).
- Reads and prints the NMI/RESET/IRQ vectors from the PRG tail.

### reorganize
- Checks the canonical directory set is present.
- Flags build artifacts (`.o`, `.class`, `.bin`, compiled binaries, example
  outputs) that must stay git-ignored.
- `--apply` creates missing standard directories; without it, report-only
  (returns non-zero if problems are found — handy before finalize).

### finalize
- Gate 1 **structure**: `nts-analyze` must succeed.
- Gate 2 **split**: `rom_split` must decompose it (vectors present).
- Gate 3 **size**: within `--max` bytes (default 1 MiB).
- On full pass, writes a `RELEASE.txt` with image, size, FNV-style fingerprint,
  gates, and `status=FINALIZED`.

## Scope

Every tool here operates on **our own** source and build outputs. None read,
extract, or reproduce another game's content — consistent with the whole
project (see [`../DESIGN.md`](../DESIGN.md) and [`../LEGALITY.md`](../LEGALITY.md)).
