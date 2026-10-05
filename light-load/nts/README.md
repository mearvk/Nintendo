# Nintendo Technical Series — structural reader & discriminator

A small C + C++ toolkit that reads the **structure** of an iNES `.nes` image
and classifies it along three axes — **merit**, **strategy**, **components** —
then evaluates five discriminator **conditions**: *play, guarantee, chapters,
win, chemistry*.

It reads **only** the 16-byte iNES header and the region layout it implies
(header / trainer / PRG-ROM / CHR-ROM). It never reads, extracts, or reproduces
game content (text, maps, CHR tiles, code). This keeps it aligned with
`light-load`'s text-only, "describe, don't distribute" policy.

## Layout

```
nts/
├── c/   ines.{h,c}            # structural parser (header → regions)
├── cpp/ discriminator.hpp     # axes + conditions API
│       conditions.cpp         # axis builders, 5 condition evaluators, renderers
│       main.cpp               # CLI
└── Makefile
```

## Build & run

```sh
cd nts && make
./nts-analyze ../NobunagaAmbition001.nes          # markdown report
./nts-analyze ../NobunagaAmbition001.nes --tsv     # one row per condition
```

## What the axes mean

- **merit** — intrinsic, measurable facts: format, PRG/CHR bank counts and
  sizes, file vs. computed size, a header fingerprint.
- **strategy** — layout/mapper consequences: mapper number, mirroring, CHR
  source (ROM vs RAM), persistence (battery), bank-switching surface.
- **components** — the enumerated physical regions with byte offsets/lengths.

## The five conditions

| condition | satisfied when |
|---|---|
| play | valid magic, PRG present, regions fit the file |
| guarantee | `computed_size == file_size` exactly |
| chapters | more than one PRG bank (switchable partitions) |
| win | mapped ∧ battery-backed ∧ guarantee |
| chemistry | CHR source agrees with bank count, regions cohere |

Thresholds/semantics live in `cpp/conditions.cpp` and are data-driven: no code
change is needed when a different image flips a condition.

## Output trees

- `../in-progress/{merit,strategy,components,rules}` — working reports.
- `../result/{merit,strategy,components,rules}` — promoted reports.

Promotion rule and the recorded outcome for this workflow (including a documented
128-byte trailer on both reference images) are in each tree's `rules/RULES.md`.
