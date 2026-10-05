# NTS discrimination rules — `result`

This is the **promoted** rule set. Reports land here only after their
conditions are reviewed per `../../in-progress/rules/RULES.md` and any failing
`guarantee` is explained. The axis definitions and condition semantics are
identical to the working tree; this file records the *accepted* outcome.

> Scope boundary (unchanged): classification uses only iNES header-derived
> structural metadata. No game content is read, extracted, or reproduced.

## Promotion decision for this workflow

Both references were promoted. Each satisfies the promotion gate
(`play ∧ chemistry`), and the `guarantee` failure is fully explained by a
documented **128-byte trailer** past the last iNES region — a benign appended
footer, not corruption or truncation.

| file | format | mapper | prg | chr | play | guarantee | chapters | win | chemistry | trailer | promoted |
|---|---|---|---|---|---|---|---|---|---|---|---|
| NobunagaAmbition001.nes | iNES | 1 (MMC1) | 256 KiB | CHR-RAM | ✅ | ❌ (128 B trailer) | ✅ | ❌ | ✅ | 128 B | ✅ |
| NobunagaAmbition002.nes | iNES | 5 (MMC5) | 256 KiB | 128 KiB | ✅ | ❌ (128 B trailer) | ✅ | ❌ | ✅ | 128 B | ✅ |

### Notes

- **`win` is false for both** by design: the gate requires exact size
  integrity (`guarantee`), which the trailer breaks. If a future variant of
  these images is trimmed to byte-exact iNES layout, `guarantee` → true and
  (both being mapped + battery-backed) `win` → true automatically. No code
  change needed; the condition is data-driven.
- The two titles differ meaningfully in **strategy**: `001` is MMC1 with
  CHR-RAM (tiles streamed at runtime); `002` is MMC5 with 128 KiB CHR-ROM
  (fixed tiles, larger bank-switching surface).
