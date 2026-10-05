# docs — building a new premium NES edition

The rules and conventions for deriving a new, original "premium edition" NES
game or cartridge, using only publicly documented tools and formats. Nothing
here involves copying an existing game; it is the standard author-your-own
pipeline.

| Document | Covers |
|---|---|
| [`FORMATS.md`](FORMATS.md) | Container (iNES vs. NES 2.0), the 16-byte header, exponent sizes, mappers (MMC1/3/5, UNROM-512), mirroring, patch formats. |
| [`ASSETS.md`](ASSETS.md) | Creating original code, graphics (CHR), and audio with our own tooling — no third-party editors or engines. |
| [`VALIDATION.md`](VALIDATION.md) | The verification series: `nts-analyze` structural checks → our own execution harness → hardware → regression. |
| [`EDITIONS.md`](EDITIONS.md) | The end-to-end checklist that ties the above together, plus the one hard rule. |

## The one hard rule

A "new edition" means **new, original authorship** — your code, art, and music
— possibly on the same *formats and mappers* the classics used, but not derived
from a copyrighted game's content. See [`../LEGALITY.md`](../LEGALITY.md).

## The repo tools that automate this

- `../nts/` — the C reader + C++ discriminator (`nts-analyze`), the C canvas
  generator (`nts-canvas`), and the Java equivalent (`java-rom-tool`).
- Acronyms used throughout: [`../ACRONYMS.md`](../ACRONYMS.md).
