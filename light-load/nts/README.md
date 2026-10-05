# Nintendo Technical Series — structural reader & discriminator

A small C + C++ toolkit that reads the **structure** of an iNES / NES 2.0
`.nes` image and classifies it along three axes — **merit**, **strategy**,
**components** — then evaluates five discriminator **conditions**, enclosed as
a **ruth** diagram: *play, guarantee, chapters, win, chemistry*.

It reads **only** the 16-byte header and the region layout it implies
(header / trainer / PRG-ROM / CHR-ROM), plus NES 2.0 size and RAM-shift fields.
It never reads, extracts, or reproduces game content (text, maps, CHR tiles,
code). This keeps it aligned with `light-load`'s text-only, "describe, don't
distribute" policy — and with the developer-friendly side of the NES case law
summarised in [`../LEGALITY.md`](../LEGALITY.md).

## Layout

```
nts/
├── c-ines-reader/      ines.{h,c}       # structural parser (header -> regions)
├── cpp-discriminator/  discriminator.hpp# axes + conditions API
│                       conditions.cpp   # axes, 5 conditions, ruth diagram, renderers
│                       main.cpp         # CLI
└── Makefile
```

## Build & run

```sh
cd nts && make
./nts-analyze ../NobunagaAmbition001.nes            # markdown report
./nts-analyze ../NobunagaAmbition001.nes --ruth      # the ruth diagram
./nts-analyze ../NobunagaAmbition001.nes --tsv       # one row per condition
```

## The three axes

- **merit** — intrinsic facts: format, PRG/CHR banks and sizes, NES 2.0 PRG-RAM/
  CHR-RAM, trailer bytes, header fingerprint.
- **strategy** — layout consequences: mapper, mirroring, CHR source (ROM vs
  RAM), persistence (battery), bank-switching surface.
- **components** — enumerated physical regions with byte offsets/lengths.

## The five conditions (the "ruth" diagram)

| condition | satisfied when |
|---|---|
| play | valid magic, PRG present, not truncated |
| guarantee | known size and not truncated (a benign trailer is allowed) |
| chapters | more than one PRG bank (switchable partitions) |
| win | mapped ∧ battery-backed ∧ guarantee |
| chemistry | CHR source agrees with bank count, not truncated |

`--ruth` prints them in a single five-cell box: `play`/`guarantee` on top,
`chapters`/`win` in the middle, `chemistry` centred below.

## Improvements in this revision

- **Trailer vs. truncation split.** A file *larger* than its declared layout is
  a benign trailer (reported, not penalised); a file *smaller* is `truncated`
  (fails `play`/`guarantee`/`chemistry`). This is why both reference ROMs now
  pass all five conditions despite a 128-byte footer.
- **NES 2.0 large-memory decoding.** PRG/CHR sizes decode exponent notation
  (`2^E·(M·2+1)`) up to the multi-megabyte range, plus PRG-RAM/CHR-RAM shift
  fields. Sizes are 64-bit throughout, so the same conditions apply to expanded
  images that newer emulators can run.

## Output trees

- `../in-progress/{merit,strategy,components,rules}` — working reports (+ `*.ruth.txt`).
- `../result/{merit,strategy,components,rules}` — promoted reports (+ `*.ruth.txt`).

Promotion rule and recorded outcomes are in each tree's `rules/RULES.md`.
