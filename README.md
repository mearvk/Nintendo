<h1 align="center">mearvk · Nintendo Studio</h1>

<p align="center">
  <strong>A professional, clean-room toolchain for reading and editing the text of Nintendo Entertainment System ROMs.</strong>
</p>

<p align="center">
  <em>Four languages. One format. Zero copyrighted content.</em>
</p>

---

## Overview

This repository is a **studio** — a disciplined home for original tooling that
works with the public, documented **iNES** ROM format. Its centerpiece is the
**Professional Editor**: a text-and-menu editor for NES ROM images, implemented
four ways (C, C++, SLeeLa, Java) that all speak one shared edit-script grammar
and produce byte-identical results.

The studio is built around a simple, honest principle:

> **We ship tools, not ROMs.** Every line of code here is original. The editor
> operates on a ROM image *you already own*; the repository stores none. The
> work is clean-room reverse-engineering of a file format, nothing more.

## The toolchain

| Component | What it is |
|---|---|
| [`professional-editor/`](professional-editor/) | The NES ROM text/menu editor — **C, C++, SLeeLa, Java** |
| [`professional-editor/docs/FORMAT.md`](professional-editor/docs/FORMAT.md) | The iNES container + the editing model |
| [`professional-editor/docs/WORKFLOW.md`](professional-editor/docs/WORKFLOW.md) | A worked dump → script → apply → verify example |

### What the editor does

- Parses the **iNES** header and PRG-ROM / CHR-ROM regions, mapper, and trainer.
- Decodes and replaces **fixed-width text slots** and named **menu-item slots**.
- Supports optional **`.tbl` character maps** so text is read and written in a
  game's own tile encoding — not just ASCII.
- Honors one safety invariant above all: **a replacement must fit its slot.**
  The editor never relocates data or rewrites pointers, so an edited ROM stays
  structurally valid. Over-length edits and failed guards abort cleanly.

### Four implementations, one grammar

```text
# the shared edit-script grammar (C · C++ · Java)
table <path.tbl>                        # optional character map
find  <offset-hex> <len> <text>         # guard: abort unless the slot matches
set   <offset-hex> <len> <text>         # replace the slot (encoded ≤ len)
menu  <name> <offset-hex> <len> <text>  # named menu-item replacement
```

The C, C++, and Java tools are verified to produce **byte-identical** output for
the same script. SLeeLa expresses the editing **model and strategy** — it
validates each planned edit against the slot invariant before a single byte is
written, then hands the approved plan to a byte tool.

## The load tiers — a text-only staging ground

Editing campaigns are organized into three **ROM-free** working areas. They hold
references, character tables, edit scripts, and notes — never game images.

| Tier | For | |
|---|---|---|
| [`light-load/`](light-load/) · [`ll/`](ll/) | one title, a few edits | the quick pass |
| [`medium-load/`](medium-load/) | a batch of titles, shared tables | the working set |
| [`heavy-load/`](heavy-load/) | large multi-title campaigns, full tables | the long haul |

Each tier carries a `manifest.tsv` that references titles **by filename only**,
plus `tables/`, `scripts/`, and `notes/`. A `.gitignore` in every tier blocks
ROM and image formats, so the "no ROMs here" promise is enforced mechanically,
not just by convention.

## Quick start

```sh
# Build the editors (C, C++, Java; SLeeLa is interpreted)
cd professional-editor && ./build-all.sh

# Inspect a ROM you own
c/nes-edit info your.nes

# Read a text slot — ASCII, or through a character table
c/nes-edit dump your.nes 1A2C 16 your.tbl

# Apply an edit script to a working copy; the original is never touched
c/nes-edit apply your.nes edits.edit your.edited.nes
```

The C++ (`cpp/nes-edit-cpp`) and Java (`NesEditCli`) front-ends take identical
arguments.

## Design principles

- **Format-faithful.** We read iNES exactly as specified and change only what we
  are asked to, in place.
- **Non-destructive.** Originals are inputs, never outputs. The slot invariant
  keeps every edit reversible and structurally safe.
- **Multi-language parity.** The same edit yields the same bytes in C, C++, and
  Java; SLeeLa reasons about the plan first.
- **Clean-room and ROM-free.** The repository is tooling and text. The games are
  yours and stay with you.

---

<p align="center"><sub>
  Professional NES ROM tooling · iNES format · clean-room · no copyrighted content
</sub></p>
