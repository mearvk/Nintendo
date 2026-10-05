# DESIGN — architecture, terms, years, definitions & scope

The design document for the `light-load` **Nintendo Technical Series (NTS)**:
an original, self-contained toolchain for *reading the structure of* and
*authoring new, original* NES software. It records the architecture, every
acronym and project term with its technology era / year and definition, and —
importantly — the **scope boundaries** that govern the whole project.

> One-line scope: we **read structure** and **author new, original content**.
> We never read, extract, or reproduce the creative content of an existing
> copyrighted game. See [`LEGALITY.md`](LEGALITY.md).

---

## 1. Scope

### In scope
- **Structural analysis** of iNES / NES 2.0 images (header + region layout
  only).
- **Classification** of that structure (merit / strategy / components; the five
  conditions; the ruth diagram).
- **Generation** of brand-new, empty NES 2.0 canvases (format-capacity).
- **Authoring** of original NES software: our own assembler, encoders, runtime
  6502 code, linker, build driver, and a validation harness.
- **Documentation** of the public formats, mappers, and the pipeline.

### Out of scope (hard boundaries)
- Reading, extracting, or reproducing **game content** (text, maps, CHR tiles,
  code) from a copyrighted ROM.
- Modifying/expanding an existing copyrighted ROM, or distributing ROM bytes.
- Any **third-party tool dependency** — every program here is our own code,
  compiling with only a C/C++ compiler and (for the Java study) the JDK.

### Legal footing (summary; not legal advice)
Structure-only reading sits in the reverse-engineering-for-interoperability
zone of *Atari Games Corp. v. Nintendo* (Fed. Cir. 1992); authoring new work
copies nothing. Full detail and caveats in [`LEGALITY.md`](LEGALITY.md).

---

## 2. Architecture overview

```
                        ┌─────────────────────────── authoring ───────────────────────────┐
  our source  ──►  asm6502 (code)      ─┐
  our tiles   ──►  chr_encode (CHR)    ─┤
  our screen  ──►  nt_build (nametable)─┼─►  rom_link  ──►  bootable .nes
  our palette ──►  pal_build (palette) ─┤   (header + PRG + CHR + vectors)
  our score   ──►  audio_build (NTSND) ─┘            │
                                                     ▼
                        ┌───────────────────── validation ─────────────────────┐
                        │  nts-analyze (structure: axes + conditions + ruth)    │
                        │  exec6502     (behaviour: 6502 stepper + golden state) │
                        └───────────────────────────────────────────────────────┘

  on-cartridge runtime (our 6502): reset ─► main ─► ppu-driver / input / audio-engine
```

### Components by directory
| Path | Language | Role |
|---|---|---|
| `nts/c-ines-reader/` | C | iNES / NES 2.0 structural parser. |
| `nts/cpp-discriminator/` | C++ | Axes, five conditions, ruth diagram, renderers. |
| `nts/c-canvas-gen/` | C | New empty NES 2.0 canvas generator. |
| `nts/java-rom-tool/` | Java | OOD study: reader + discriminator + canvas in one. |
| `tools/asm6502/` | C | Two-pass 6502 assembler. |
| `tools/chr-encoder/` | C | 8×8 tile text → NES two-bit-plane CHR. |
| `tools/audio-builder/` | C | Text score → `NTSND v1` byte stream. |
| `tools/nametable-builder/` | C | Screen text → 1 KiB nametable. |
| `tools/palette-tool/` | C | Palette text → validated 32-byte image. |
| `tools/rom-linker/` | C | Assemble bootable `.nes` with interrupt vectors. |
| `tools/exec-harness/` | C | Minimal deterministic 6502 stepper for validation. |
| `runtime/reset|ppu-driver|input|audio-engine/` | 6502 asm | On-cartridge runtime. |
| `build/` | C++ | One-command build driver + demo project. |
| `docs/` | — | FORMATS / ASSETS / VALIDATION / EDITIONS. |

### Why three languages (C / C++ / Java)
The same domain is modeled three ways as a comparative object-oriented-design
study: **C** (procedural, closest to hardware), **C++** (value types +
namespaces over the C core), **Java** (classes/enums/records; clearest OOD).

---

## 3. Acronyms & terms — era, year, definition

> "Year" is the earliest widely-cited date for the term or standard. Community
> formats (NES 2.0, UNROM-512, MSU-1, our NTSND) use approximate first dates.
> A fuller list with sources lives in [`ACRONYMS.md`](ACRONYMS.md); this table
> is the design-level reference.

### Platform & hardware
| Term | Era | Year | Definition / scope here |
|---|---|---|---|
| **NES** | 8-bit console | 1983 JP / 1985 NA | Nintendo Entertainment System; the target platform. |
| **CPU** | microprocessor | 1975-era core | The NES processor (Ricoh 2A03, 6502 derivative). |
| **PPU** | graphics chip | 1983 | Picture Processing Unit; renders tiles/sprites. Driven by `runtime/ppu-driver`. |
| **APU** | audio unit | 1983 | Audio Processing Unit; sound registers. Driven by `runtime/audio-engine`. |
| **OAM** | PPU memory | 1983 | Object Attribute Memory; sprite table, filled via DMA ($4014). |
| **DMA** | transfer | 1983 (NES use) | Direct Memory Access; bulk copy to OAM. |
| **VS / PlayChoice** | arcade | 1984 / 1986 | NES-derived arcade console types (header flags). |

### Container & file formats
| Term | Era | Year | Definition / scope here |
|---|---|---|---|
| **iNES** | emulation | 1996 | De-facto NES ROM container: 16-byte header + PRG/CHR. Parsed by `c-ines-reader`. |
| **NES 2.0** | emulation | 2006 | Backward-compatible header extension: submappers, large (exponent) sizes, RAM-shift fields. The standard we author to. |
| **IPS / BPS** | patch | 1990s / 2010s | Patch formats (ship a diff, not the game). Referenced only. |
| **NTSND v1** | this project | 2026 | *Our own* audio byte stream: `53 01`, 3-byte events `[chan][semitone][frames]`, `FF` terminator. Produced by `audio-builder`, consumed by `runtime/audio-engine`. |
| **ineshdr** | this project | 2026 | *Our* file extension for a 16-byte header-only capacity declaration (not a ROM). |

### Memory regions & mapper chips
| Term | Era | Year | Definition / scope here |
|---|---|---|---|
| **ROM** | memory | 1960s | Read-Only Memory; fixed cartridge data. |
| **RAM** | memory | 1960s | Random-Access (writable) memory. |
| **PRG** | NES cartridge | 1983 | Program ROM/RAM; executable 6502 code/data region. |
| **CHR** | NES cartridge | 1983 | Character ROM/RAM; graphics tile (pattern) region. CHR-ROM fixed; CHR-RAM runtime-written. |
| **WRAM** | NES cartridge | 1986 | Work RAM; on-cart, often battery-backed for saves. |
| **MMC** | NES cartridge | 1986 | Memory Management Controller; Nintendo's mapper chip family (bank switching). |
| **MMC1 / MMC3 / MMC5** | NES cartridge | 1987 / 1988 / 1988 | Mappers 1 / 4 / 5; increasing capability (MMC5 most capable). Target specs we build *to*. |
| **NROM** | NES cartridge | 1983 | Mapper 0; no bank switching. Our demo image. |
| **Mapper 30 (UNROM-512)** | homebrew | 2012 | 512 KB self-flashing board spec; large "premium" target. |
| **MSU-1** | enhancement | 2012 | Streamed-audio expansion (SNES origin); referenced as a concept only. |

### Units & software terms
| Term | Era | Year | Definition / scope here |
|---|---|---|---|
| **KB / MB** | computing | 1960s–70s | Kilobyte / Megabyte (decimal-ish, as commonly written). |
| **KiB / MiB** | computing | 1998 (IEC) | Binary units: 2^10 / 2^20 bytes. Used for exact region sizes. |
| **NTS** | this project | 2026 | Nintendo Technical Series; the name of this toolchain. |
| **CLI** | software | 1960s | Command-Line Interface; how every tool here is invoked. |
| **TSV** | data | 1970s | Tab-Separated Values; `nts-analyze --tsv` machine output. |
| **FNV** | hashing | 1991 | Fowler–Noll–Vo-style accumulator; our header fingerprint (stable, not cryptographic). |
| **OOD** | software | 1980s–90s | Object-Oriented Design; the comparative study across C/C++/Java. |
| **ISA** | computing | 1960s | Instruction Set Architecture; the 6502 ISA our `asm6502` subset targets. |
| **NMI / IRQ / RESET** | CPU | 6502-era | Interrupt vectors at `$FFFA/$FFFC/$FFFE`; written by `rom_link` so an image boots. |

### Project-specific analytic terms
| Term | Definition |
|---|---|
| **merit** | Axis: intrinsic, measurable structural facts (format, bank counts/sizes, fingerprint). |
| **strategy** | Axis: layout/mapper consequences (mapper, mirroring, CHR source, persistence, bank surface). |
| **components** | Axis: enumerated physical regions with byte offsets. |
| **play** | Condition: valid magic, PRG present, not truncated. |
| **guarantee** | Condition: known size, not truncated (a documented trailer is allowed). |
| **chapters** | Condition: more than one PRG bank (switchable partitions). |
| **win** | Condition: mapped ∧ battery-backed ∧ guarantee. |
| **chemistry** | Condition: CHR source agrees with bank count; regions cohere. |
| **ruth** | The five-cell diagram enclosing the five conditions (`--ruth`). |
| **trailer** | Bytes present *past* the last declared region (benign). |
| **truncated** | Declared layout *larger* than the file (corruption). |
| **exponent notation** | NES 2.0 size encoding `2^E·(M·2+1)` for large PRG/CHR. |

---

## 4. Data formats we define

### iNES / NES 2.0 header (16 bytes) — what we read/write
```
[0..3] 'N''E''S' 0x1A      magic
[4]    PRG low byte        (16 KiB units; NES 2.0 exponent if hi nibble = 0x0F)
[5]    CHR low byte        (8 KiB units;  NES 2.0 exponent if hi nibble = 0x0F)
[6]    flags6: mirroring, battery, trainer, mapper low nibble
[7]    flags7: console type, NES 2.0 id (bits 2-3 = 10), mapper mid nibble
[8]    NES 2.0: mapper high nibble + submapper
[9]    NES 2.0: PRG/CHR size high nibbles
[10]   NES 2.0: PRG-RAM / PRG-NVRAM shift counts
[11]   NES 2.0: CHR-RAM / CHR-NVRAM shift counts
[12..15] timing / console / misc
```

### Interrupt vector table (last 6 bytes of PRG) — written by `rom_link`
```
$FFFA/B  NMI    (little-endian address)
$FFFC/D  RESET  (CPU jumps here on power-on)
$FFFE/F  IRQ/BRK
```

### NTSND v1 (our audio stream)
```
byte 0 : 'S' (0x53)        magic
byte 1 : 0x01              version
events : [channel 0-3][semitone 0=rest,1..96][frames 1..255]  (3 bytes each)
end    : 0xFF
```

### CHR tile (16 bytes/tile) — produced by `chr_encode`
Two bit-planes per 8×8 tile: plane 0 (low bits) then plane 1 (high bits),
row by row. Pixel value 0..3 = palette index.

---

## 5. Pipeline & verification series

**Author → Build → Validate → Package** (full checklist in
[`docs/EDITIONS.md`](docs/EDITIONS.md)):

1. **Author** original code/tiles/screen/palette/score with our tools.
2. **Build** via `build/build_driver` (manifest-driven) → `rom_link` emits a
   bootable `.nes` with correct vectors.
3. **Validate**:
   - *structure* — `nts-analyze`: `play`, `chemistry`, clean `guarantee` must
     hold; verdict shown as the ruth diagram.
   - *behaviour* — `exec6502`: run from RESET, assert golden state (the demo
     reaches boot marker `A=$5A` after PPU warm-up + RAM clear).
4. **Package** the premium bundle (original art/manual/notes; all our IP).

---

## 6. Known limits & next steps

- `asm6502` implements a **subset** of the 6502 ISA; extend the `ISA[]` table
  (e.g. `SEI`, `PHA`, stack ops) as the runtime grows.
- `exec6502` models **just enough** hardware (PPU warm-up poll, controller
  reads as 0) to validate boot flow — it is a validation stepper, not a full
  emulator. Extending the PPU/APU models is the path to validating richer
  main-loop code.
- Large canvases (200 MB+) are **format-capacity** demonstrations, not runnable
  images; real mapper hardware caps out far lower.

---

## 7. Related documents

- [`README.md`](README.md) — folder overview and ROM-free policy.
- [`ACRONYMS.md`](ACRONYMS.md) — fuller acronym glossary with sources.
- [`LEGALITY.md`](LEGALITY.md) — legal footing and caveats.
- [`docs/`](docs/) — FORMATS, ASSETS, VALIDATION, EDITIONS.
- Tool/runtime READMEs under [`nts/`](nts/), [`tools/`](tools/README.md),
  [`runtime/`](runtime/README.md), [`build/`](build/README.md).
