# NES ROM Format — Editor Reference

This document describes the file format our editor operates on. The editor reads
and writes **iNES** `.nes` files, the de-facto container for NES program data.
It ships **no copyrighted ROM content** — it is a tool that edits a ROM image
the user already owns.

## 1. The iNES container

An iNES file is a 16-byte header followed by optional trainer, PRG-ROM, and
CHR-ROM banks.

```
offset  size   field
0..3    4      magic: 0x4E 0x45 0x53 0x1A  ("NES\x1A")
4       1      PRG-ROM size in 16 KiB units  (prg_banks)
5       1      CHR-ROM size in 8 KiB units   (chr_banks; 0 => CHR-RAM)
6       1      flags6  (mapper low nibble, mirroring, battery, trainer)
7       1      flags7  (mapper high nibble, NES 2.0 marker in bits 2-3)
8       1      flags8  (mapper/submapper or PRG-RAM size)
9       1      flags9  (TV system / upper ROM-size bits in NES 2.0)
10      1      flags10 (unofficial)
11..15  5      padding (zero in iNES; used by NES 2.0)
```

Then, in order:

```
trainer   512 bytes   only if (flags6 & 0x04)
PRG-ROM   prg_banks * 16384 bytes
CHR-ROM   chr_banks * 8192 bytes
```

PRG-ROM holds the program code **and the game's text/menu strings**. CHR-ROM
holds pattern (tile) data. Our text/menu editing operates on the PRG-ROM region;
the editor exposes absolute file offsets so edits are precise and reversible.

## 2. What the editor edits

The editor is deliberately format-faithful and non-destructive:

1. **Text strings** — locate and replace in-ROM text. NES games rarely store
   plain ASCII; most use a game-specific *character map* (a mapping from tile
   index -> glyph). The editor supports:
   - **ASCII mode** — for ROMs that happen to store ASCII (and for tooling/tests).
   - **Table mode** — a `.tbl` character map (the community-standard "Thingy"
     table format: `HH=c` lines) so text is searched/edited in the game's own
     encoding.
2. **Menu items** — menus are just text at known offsets; the editor treats a
   "menu item" as a labeled, length-bounded string slot so a replacement cannot
   overflow its region (critical: NES text is fixed-width in ROM).

### The length rule (safety invariant)

A replacement string, once encoded, **must not exceed the original slot
length**. Shorter replacements are padded with the table's space/filler byte.
This keeps every pointer and bank boundary intact — the editor never relocates
data or rewrites pointers (that is a per-game concern beyond a generic editor).

## 3. The `.tbl` character map

```
# comment lines start with '#'
20= 
41=A
42=B
...
/FF         # optional: control-code / line-break annotations after a slash
```

Each line is `HH=<glyph>` where `HH` is a hex byte and `<glyph>` is the single
character (or token) it renders as. The editor builds both directions:
glyph->byte for encoding, byte->glyph for decoding.

## 4. The edit script (portable across all four implementations)

All four implementations (C, C++, SLeeLa, Java) accept the same line-oriented
**edit script**, so an edit is reproducible regardless of language:

```
# comments allowed
table <path.tbl>                 # optional: use a character map
find  <offset-hex> <len> <old>   # assert the slot currently decodes to <old>
set   <offset-hex> <len> <new>   # replace the slot's text with <new> (<= len)
menu  <name> <offset-hex> <len> <new>   # named menu-item replacement (same rule)
```

- `find` is an optional guard: if the slot does not currently decode to `<old>`,
  the edit aborts (so you never silently patch the wrong offset).
- `set` / `menu` replace in place, padded to `len`.

## 5. Strategy — editing "thinking games"

Strategy / puzzle / RPG titles ("thinking games") are the most text-heavy and
the best fit for this editor: menus, item names, spell lists, dialogue, and
map labels are all fixed-width PRG-ROM strings. The recommended workflow:

1. **Dump** the PRG-ROM text with a `.tbl` to find the strings and offsets.
2. Build an **edit script** of `set`/`menu` lines.
3. **Apply** it to a working copy, keeping the original untouched.
4. **Verify** by dumping again; the length invariant guarantees the ROM stays
   structurally valid.

See [`WORKFLOW.md`](WORKFLOW.md) for a worked example on a synthetic sample ROM.
