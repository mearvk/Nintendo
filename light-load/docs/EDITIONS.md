# Editions — the rules for deriving a new premium NES edition

A single checklist tying together `FORMATS.md`, `ASSETS.md`, and
`VALIDATION.md`. Follow it top to bottom to produce a new, original "premium
edition" game or cartridge **using only our own code** against the public NES
2.0 format. No third-party tools.

## The one hard rule

A "new edition" means **new, original authorship** — our code, our art, our
music, and our tools — built on the public *format and mapper facts* the
classics used, but **not derived from a copyrighted game's content**. Formats,
mapper numbers, and header fields are facts we may freely target; another
studio's PRG/CHR data is not. (See `../LEGALITY.md`.)

## The pipeline / checklist

### 1. Format & mapper (see FORMATS.md)
- [ ] Target the **NES 2.0** header.
- [ ] Choose a mapper target matching ambition (e.g. mapper 1 / 4 / 5 / 30);
      these are hardware facts we build *to*, not tools we depend on.
- [ ] Declare only what the target hardware actually runs (KB–low-MB).

### 2. Our own tooling & assets (see ASSETS.md)
- [ ] Write the 6502 program ourselves (direct byte emission or our own
      assembler).
- [ ] Generate original CHR tiles with our own encoder.
- [ ] Author original audio data and our own playback engine.
- [ ] Keep source + data in VCS; the ROM is a build output of our code.

### 3. Build
- [ ] Produce the ROM from our source.
- [ ] Lay out the memory map / header with `nts-canvas` when designing the
      footprint first.

### 4. Validate (see VALIDATION.md)
- [ ] `nts-analyze` → `play`, `chemistry`, clean `guarantee` all pass.
- [ ] Behaviour verified with **our own execution harness** against our
      expected-state fixtures.
- [ ] (Optional) runs on target hardware via our own flash/verify routine.
- [ ] `--tsv` regression diff vs. previous build.

### 5. Package the premium bundle
- [ ] Original box/label/manual art we created.
- [ ] Optional runtime presentation layer written by us.
- [ ] Edition notes / changelog.
- [ ] License the bundle as we choose — it is wholly our IP, tools included.

## What each repo tool contributes

| Our tool (C / C++ / Java) | Role in the pipeline |
|---|---|
| `nts-analyze` (reader + discriminator) | Stage 4 structural validation. |
| `nts-canvas` (generator) | Stage 3 memory-map/header baseline. |
| Java `nts` tool | Same read + generate, for OOD study and JVM pipelines. |

## Why three languages (C / C++ / Java)

The same domain — NES 2.0 structure, the three axes, the five conditions, the
canvas generator — is modeled three ways on purpose, as a comparative
object-oriented-design study:
- **C** — procedural, explicit memory, closest to the hardware.
- **C++** — value types + namespaces, a thin typed layer over the C core.
- **Java** — classes, enums, and records; the clearest OOD expression and an
  easy fit for JVM build pipelines.

All three are **our** code with no external dependencies — compile with a C/C++
compiler and the JDK, nothing else.
