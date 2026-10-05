# Formats — the container and mapper rules for a new NES edition

The publicly documented formats a new, original "premium edition" NES game is
built on. These are facts about the platform you may freely use; they are not
anyone's copyrighted content.

> Scope: this project reads and *generates* these formats. It never reproduces
> the content of an existing game.

## The container: iNES vs. NES 2.0

| | iNES (1996) | NES 2.0 (2006) |
|---|---|---|
| Header | 16 bytes | 16 bytes (backward compatible) |
| Max PRG/CHR | 4 MB / 2 MB (12-bit bank counts) | multi-megabyte via exponent notation |
| Submappers | no | yes |
| RAM declared | implicit | explicit PRG-RAM / CHR-RAM shift fields |
| Use for a premium edition | legacy only | **yes — the standard** |

**Rule:** author to **NES 2.0**. It is backward compatible and unlocks large
sizes, submappers, and precise RAM declarations.

### The 16-byte header (both formats)

```
[0..3] 'N' 'E' 'S' 0x1A         magic
[4]    PRG-ROM size, low byte   (16 KiB units; or NES 2.0 exponent)
[5]    CHR-ROM size, low byte   (8 KiB units;  or NES 2.0 exponent)
[6]    flags6: mirroring, battery, trainer, mapper low nibble
[7]    flags7: console type, NES 2.0 id (bits 2-3 = 10), mapper mid nibble
[8]    NES 2.0: mapper high nibble + submapper
[9]    NES 2.0: PRG/CHR size high nibbles
[10]   NES 2.0: PRG-RAM / PRG-NVRAM shift counts
[11]   NES 2.0: CHR-RAM / CHR-NVRAM shift counts
[12..15] timing / console / misc
```

### NES 2.0 large-memory (exponent) notation

When a size field's high nibble is `0x0F`, the low byte is read as:

```
size = 2^E * (M*2 + 1)     E = 6-bit exponent, M = 2-bit multiplier
```

This is what lets a header declare very large PRG/CHR. Our `nts-canvas` uses it;
`nts-analyze --declare` decodes it.

## Mappers — choosing the board

| Mapper | No. | Era | Why pick it for a new edition |
|---|---|---|---|
| **NROM** | 0 | 1983 | Tiny games, no bank switching. Learning only. |
| **MMC1** | 1 | 1987 | Classic 128–256 KB; battery saves. |
| **MMC3** | 4 | 1988 | Workhorse: scanline IRQ for split status bars; a common target. |
| **MMC5** | 5 | 1988 | Most capable Nintendo mapper: extra RAM, fine banking, extra audio. |
| **Mapper 30** | 30 | 2012 | A 512 KB self-flashing board spec; a large, "premium"-scale target. |

These are **hardware facts we build *to*** — target specifications, not tools we
depend on. Our own code declares and drives the chosen mapper.

**Rules of thumb**
- Match the mapper to the ambition; do not over-declare.
- A shippable edition stays in the KB–low-MB range (what the target runs).
- The 200 MB+ canvas this repo can emit is a **format-capacity** demonstration,
  not a runnable ROM.

## Mirroring & RAM

- **Mirroring** (horizontal / vertical / four-screen) sets nametable layout and
  drives your scroll strategy. Declared in flags6.
- **Battery-backed WRAM** gives saved games; declare it in flags6 (and size via
  the NES 2.0 RAM-shift fields).

## Patch formats (for shipping changes, not games)

| Format | Era | Use |
|---|---|---|
| **IPS** | 1990s | Simple binary patch; ships a diff, not the game bytes. |
| **BPS** | 2010s | Modern, checksum-verified patch. |

**Rule:** if you ever distribute modifications to something you own, ship a
*patch*, never the game image. (A community convention, not a legal safe harbor —
see `../LEGALITY.md`.)
