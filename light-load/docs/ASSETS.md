# Assets — original content creation for a new NES edition

The publicly known tools and conventions for authoring the **original** code,
graphics, and audio that make up a new "premium edition." Everything here is
about creating your own content — the point of a new edition is that you own
every byte.

## Toolchains (code)

| Tool | Era | What it is | Use |
|---|---|---|---|
| **cc65 / ca65** | long-standing | C compiler + 6502 assembler suite | The standard way to write NES code in C and/or assembly. |
| **NESFab** | modern | A language + compiler built for the NES | Efficient NES code with higher-level ergonomics. |
| **NESmaker** | modern | GUI game-building toolkit | Build complete games on modern mappers without hand-writing assembly. |
| **asm6 / nesasm** | classic | Lightweight 6502 assemblers | Minimal, dependency-free assembly builds. |

**Rule:** keep **source + assets** in version control; the ROM is a **build
output**, never a committed artifact. (Same discipline `light-load` enforces for
ROM bytes.)

## Graphics (CHR)

| Tool | What it does |
|---|---|
| **YY-CHR** | Classic tile/CHR editor. |
| **NEXXT / NESST** | Modern NES-specific tile, nametable, and palette editors. |
| **Aseprite** (+ export) | General pixel-art tool; export to NES-constrained palettes/tiles. |

**Rules**
- Author original tiles/sprites only — original characters, not reproductions.
- Respect the hardware palette and the 8×8 / 8×16 sprite constraints.
- CHR-ROM = fixed tiles; CHR-RAM = tiles streamed at runtime (your mapper choice
  decides which).

## Audio

| Tool | What it does |
|---|---|
| **FamiStudio** | Modern NES music tracker; exports data your engine plays back. |
| **FamiTracker** | Classic NES tracker. |
| **FamiTone2 / FamiStudio sound engine** | Playback engines you link into your ROM. |

**Rules**
- Compose original music/SFX.
- Expansion audio (VRC6, MMC5, FDS, Namco 163) is available only if your mapper
  and target support it — declare and test accordingly.

## Metadata & "premium" packaging assets

A premium *edition* is a bundle, all original IP you create:
- Box art, manual, and label art (your own artwork).
- Optional **Mesen HD Pack** (high-resolution art overlay) — see `VALIDATION.md`
  and `EDITIONS.md`.
- A written changelog / edition notes.

**Rule:** every asset in the bundle is your own authorship, licensable as you
choose.
