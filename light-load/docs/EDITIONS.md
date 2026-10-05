# Editions — the rules for deriving a new premium NES edition

A single checklist that ties together `FORMATS.md`, `ASSETS.md`, and
`VALIDATION.md`. Follow it top to bottom to produce a new, original "premium
edition" game or cartridge, using only publicly documented tools and formats.

## The one hard rule

A "new edition" means **new, original authorship** — your code, your art, your
music — possibly built on the same *formats and mappers* the classics used, but
**not derived from a copyrighted game's content**. Formats, mapper numbers, and
header fields are facts you may freely use; another studio's PRG/CHR data is
not. (See `../LEGALITY.md`: this is the line *Atari v. Nintendo* and *Galoob v.
Nintendo* draw.)

## The pipeline / checklist

### 1. Format & mapper (see FORMATS.md)
- [ ] Target **NES 2.0** header.
- [ ] Pick a mapper matching ambition: **UNROM-512 (30)** for publishable
      homebrew, **MMC3 (4)** for IRQ split screens, **MMC5 (5)** for maximum
      capability.
- [ ] Declare only what hardware/emulators actually run (KB–low-MB).

### 2. Toolchain & assets (see ASSETS.md)
- [ ] Code in **cc65/ca65**, **NESFab**, or **NESmaker**.
- [ ] Original CHR via **YY-CHR / NEXXT / NESST**.
- [ ] Original audio via **FamiStudio** (+ sound engine).
- [ ] Keep source + assets in VCS; ROM is a build output, never committed.

### 3. Build
- [ ] Produce the ROM from source.
- [ ] Generate a header/capacity baseline with `nts-canvas` if designing the
      memory map first.

### 4. Validate (see VALIDATION.md)
- [ ] `nts-analyze` → `play`, `chemistry`, clean `guarantee` all pass.
- [ ] Boots on **Mesen** + one other accurate emulator.
- [ ] (Optional) runs on a real **UNROM-512** flash cart.
- [ ] `--tsv` regression diff vs. previous build.

### 5. Package the premium bundle
- [ ] Original box/label/manual art.
- [ ] Optional **Mesen HD Pack** (runtime overlay — *Galoob*-style enhancement,
      original assets only).
- [ ] Edition notes / changelog.
- [ ] License the bundle as you choose — it is wholly your IP.

## What each repo tool contributes

| Tool (C / C++ / Java) | Role in the pipeline |
|---|---|
| `nts-analyze` (reader + discriminator) | Stage 4 structural validation. |
| `nts-canvas` (generator) | Stage 3 memory-map/header baseline. |
| Java `nts` tool | Same read + generate, for OOD study and JVM pipelines. |

## Why three languages (C / C++ / Java)

The same domain — NES 2.0 structure, the three axes, the five conditions, the
canvas generator — is modeled three ways on purpose, as a comparative
object-oriented-design study:
- **C** — procedural, explicit memory, the closest to the hardware.
- **C++** — value types + namespaces, a thin typed layer over the C core.
- **Java** — classes, enums, and interfaces; the clearest OOD expression and an
  easy fit for JVM build pipelines and tooling.

Studying one well-specified format across three paradigms is a practical way to
learn where OOD helps (modeling, extensibility) and where it costs
(indirection, allocation) — useful background for anyone building commercial
tooling in this space.
