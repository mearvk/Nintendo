# Assets — creating original content with our own code

How we author the **original** code, graphics, and audio for a new edition —
using **only our own tooling** built against the format. No third-party
compilers, editors, trackers, or engines. The point of a new edition is that we
own every byte, including the tools that make it.

> No external dependencies. Everything here is produced by code we write in
> this repo (C / C++ / Java) operating directly on the NES 2.0 format described
> in `FORMATS.md`.

## Code

We write the game's 6502 program ourselves. Two paths, both ours:

- **Hand-authored machine code / assembly** — emit 6502 opcodes directly into
  the PRG region. Our generators already write exact bytes at exact offsets;
  the same approach places code, not just zeros.
- **Our own assembler/codegen** — if we want symbolic assembly, we write the
  assembler. It is a text-to-bytes pass over our own mnemonic table; the
  canvas/region model in `nts/` already gives us the layout to target.

**Rule:** source + byte tables live in version control; the ROM is a build
output of **our** code, never a committed artifact and never produced by an
outside tool.

## Graphics (CHR)

CHR tiles are a fixed, well-documented bit layout (two bit-planes per 8×8
tile). We generate them ourselves:

- Define tiles as data in our source (per-pixel palette indices) and have our
  own encoder pack them into the CHR bit-plane format.
- Author or convert our own pixel data with code we write — a small routine
  that reads our image representation and emits CHR bytes.

**Rules**
- Original tiles/sprites only — original characters, authored by us.
- Respect the hardware palette and 8×8 / 8×16 sprite constraints in our encoder.
- CHR-ROM = fixed tiles; CHR-RAM = tiles our code streams at runtime (mapper
  choice decides which).

## Audio

Sound is register writes to the APU on a timer. We own the whole chain:

- Represent music/SFX as our own data format (note/duration/channel tables).
- Write our own playback engine (the code that pushes APU register writes each
  frame) and link it into our PRG.

**Rules**
- Original compositions and our own engine — no external trackers or sound
  libraries.
- Expansion audio (where a mapper provides it) is driven by our own code if we
  choose to use it.

## Metadata & "premium" packaging

A premium *edition* is a bundle, all original IP we create ourselves:

- Box/label/manual artwork we author.
- An optional runtime presentation layer written by us (see `VALIDATION.md` /
  `EDITIONS.md`).
- Edition notes / changelog.

**Rule:** every asset — and every tool that made it — is our own authorship,
licensable as we choose.
