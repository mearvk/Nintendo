# NTS discrimination rules — `in-progress`

This document defines, in precise structural terms, how the Nintendo Technical
Series tooling classifies an iNES / NES 2.0 image. It governs the **working**
tree; a report is promoted to `../../result/` only once its conditions are
reviewed.

> Scope boundary: every rule below reads **only** header-derived structural
> metadata (sizes, bank counts, mapper, flags, RAM-shift fields, region
> offsets, and a header fingerprint). No game content — text, maps, CHR tiles,
> or code — is read, extracted, or reproduced. This matches `light-load`'s
> text-only policy.

## Axes

| axis | question it answers | source |
|---|---|---|
| **merit** | *What is intrinsically, measurably true of the image?* | header fields, sizes, NES 2.0 RAM, trailer, fingerprint |
| **strategy** | *What does the layout imply for how the image is used?* | mapper, mirroring, CHR source, persistence, bank surface |
| **components** | *What physical regions make up the image?* | region table (header / trainer / prg / chr) |

## The "ruth" diagram

The five conditions are enclosed as a single five-cell **ruth** diagram
(`nts-analyze <file> --ruth`): `play` and `guarantee` on the top row,
`chapters` and `win` on the middle row, `chemistry` centred below. Each cell
shows `[x]` (satisfied) or `[ ]` (not). It is the compact verdict view of the
discriminator.

## Conditions (predicates over structural metadata)

| condition | satisfied when | interpretation |
|---|---|---|
| **play** | valid magic, `prg_banks > 0`, and **not truncated** | the image is structurally playable |
| **guarantee** | file has a known size and is **not truncated** (a benign trailer is allowed and reported) | integrity holds; no declared region overruns the file |
| **chapters** | `prg_banks > 1` | content is partitioned into addressable, switchable "chapters" |
| **win** | mapped ∧ battery-backed ∧ `guarantee` | a terminal/completable configuration |
| **chemistry** | CHR source agrees with bank count (`CHR-RAM ⇔ chr_banks==0`) and not truncated | cross-region coherence |

### Trailer vs. truncation (improved)

Earlier the single "exact size" test conflated two opposite states. They are
now split:

- **truncated** — declared layout is *larger* than the file. Real corruption.
  Fails `play`, `guarantee`, and `chemistry`.
- **trailer** — file is *larger* than the declared layout. A benign appended
  footer. Reported as a byte count in the `merit` axis; does **not** fail
  `guarantee`.

### NES 2.0 large-memory support (improved)

The reader now decodes NES 2.0 sizes, including **exponent notation**
(`size = 2^E · (M·2+1)`), so expanded images scale to the multi-megabyte
PRG/CHR that modern emulators handle, plus the PRG-RAM/CHR-RAM shift fields.
Sizes are 64-bit throughout.

### Promotion rule (`in-progress` → `result`)

Promote when **`play` ∧ `chemistry`** hold and any trailer is documented in the
report. `win` is informational.

## Observed on this workflow's references

With the corrected logic, both references satisfy **all five** conditions. Each
carries a documented **128-byte trailer**, now treated as benign rather than a
`guarantee` failure.

| file | format | mapper | prg | chr | play | guarantee | chapters | win | chemistry | trailer |
|---|---|---|---|---|---|---|---|---|---|---|
| NobunagaAmbition001.nes | iNES | 1 (MMC1) | 256 KiB | CHR-RAM | ✅ | ✅ | ✅ | ✅ | ✅ | 128 B |
| NobunagaAmbition002.nes | iNES | 5 (MMC5) | 256 KiB | 128 KiB | ✅ | ✅ | ✅ | ✅ | ✅ | 128 B |
