# NTS discrimination rules — `result`

This is the **promoted** rule set. Reports land here only after their
conditions are reviewed per `../../in-progress/rules/RULES.md` and any trailer
is documented. Axis definitions and condition semantics are identical to the
working tree; this file records the *accepted* outcome.

> Scope boundary (unchanged): classification uses only header-derived
> structural metadata. No game content is read, extracted, or reproduced.

## The "ruth" diagram

Each promoted report ships with its five-cell **ruth** diagram
(`*.ruth.txt`): `play` / `guarantee` on top, `chapters` / `win` in the middle,
`chemistry` centred below, each cell flagged `[x]` or `[ ]`.

## Promotion decision for this workflow

Both references were promoted and now satisfy **all five** conditions. The
128-byte trailer on each is recorded as a benign appended footer, not
corruption — the trailer/truncation split means it no longer fails
`guarantee`.

| file | format | mapper | prg | chr | play | guarantee | chapters | win | chemistry | trailer | promoted |
|---|---|---|---|---|---|---|---|---|---|---|---|
| NobunagaAmbition001.nes | iNES | 1 (MMC1) | 256 KiB | CHR-RAM | ✅ | ✅ | ✅ | ✅ | ✅ | 128 B | ✅ |
| NobunagaAmbition002.nes | iNES | 5 (MMC5) | 256 KiB | 128 KiB | ✅ | ✅ | ✅ | ✅ | ✅ | 128 B | ✅ |

### Notes

- **`win` is now true for both**: each is mapped, battery-backed, and passes
  the trailer-tolerant `guarantee`.
- The two titles still differ in **strategy**: `001` is MMC1 + CHR-RAM (tiles
  streamed at runtime); `002` is MMC5 + 128 KiB CHR-ROM (fixed tiles, larger
  bank-switching surface).
- The reader is NES 2.0 large-memory aware (exponent sizes + RAM-shift fields),
  so the same conditions apply unchanged to expanded multi-megabyte images.
