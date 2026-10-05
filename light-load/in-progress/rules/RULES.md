# NTS discrimination rules — `in-progress`

This document defines, in precise structural terms, how the Nintendo Technical
Series tooling classifies an iNES image. It governs the **working** tree; a
report is promoted to `../../result/` only once its conditions are reviewed.

> Scope boundary: every rule below reads **only** iNES header-derived
> structural metadata (sizes, bank counts, mapper, flags, region offsets, and a
> header fingerprint). No game content — text, maps, CHR tiles, or code — is
> read, extracted, or reproduced. This matches `light-load`'s text-only policy.

## Axes

| axis | question it answers | source |
|---|---|---|
| **merit** | *What is intrinsically, measurably true of the image?* | header fields, sizes, fingerprint |
| **strategy** | *What does the layout imply for how the image is used?* | mapper, mirroring, CHR source, persistence, bank surface |
| **components** | *What physical regions make up the image?* | region table (header / trainer / prg / chr) |

## Conditions (predicates over structural metadata)

Each condition is a boolean derived from the axes. The phrasing maps the
developer taxonomy — *play, guarantee, chapters, win, chemistry* — onto
checkable structural facts.

| condition | satisfied when | interpretation |
|---|---|---|
| **play** | valid magic, `prg_banks > 0`, and all regions fit within the file | the image is structurally playable |
| **guarantee** | `computed_size == file_size` exactly | integrity is guaranteed: no slack, no truncation |
| **chapters** | `prg_banks > 1` | content is partitioned into addressable, switchable "chapters" |
| **win** | mapped **and** battery-backed **and** `guarantee` holds | a terminal/completable configuration (progress can persist, layout is exact) |
| **chemistry** | CHR source agrees with bank count (`CHR-RAM ⇔ chr_banks==0`) and regions fit | cross-region coherence — the parts cohere into a whole |

### Promotion rule (`in-progress` → `result`)

A report is promoted when **`play` ∧ `chemistry`** hold (the image is coherent
and usable) and any failing **`guarantee`** has a *documented explanation*
(e.g. a known trailer). `win` is informational, not a gate.

## Observed on this workflow's references

Both referenced titles parse as valid iNES images and satisfy `play`,
`chapters`, and `chemistry`. Both **fail `guarantee`** for the same reason: a
**128-byte trailer** sits past the last declared region (file is 128 B larger
than `header + prg + chr`). This is a documented, benign discrepancy (an
appended footer), so under the promotion rule the reports are eligible for
`result/` with the trailer noted.

| file | format | mapper | prg | chr | play | guarantee | chapters | win | chemistry | trailer |
|---|---|---|---|---|---|---|---|---|---|---|
| NobunagaAmbition001.nes | iNES | 1 (MMC1) | 256 KiB | CHR-RAM | ✅ | ❌ | ✅ | ❌ | ✅ | 128 B |
| NobunagaAmbition002.nes | iNES | 5 (MMC5) | 256 KiB | 128 KiB | ✅ | ❌ | ✅ | ❌ | ✅ | 128 B |
