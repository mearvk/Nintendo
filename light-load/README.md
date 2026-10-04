# light-load

`light-load` is a lightweight, **text-only** working area for the
`mearvk/Nintendo` studio. It holds **no ROM data** — only references, notes, and
small working files that are cheap to clone and safe to share. (It is the
full-name companion to the short `ll/` folder.)

> **No ROMs here.** This folder deliberately contains no `.nes` / `.zip` game
> images. It records *which* titles a workflow targets by filename and keeps the
> accompanying editor artifacts (character tables, edit scripts, notes). The
> actual ROM you work on stays local to your machine — you already own it — and
> is never committed here.

## What goes in `light-load`

- **`manifest.tsv`** — a text index of titles a workflow references, by filename
  and location. Names only; no ROM bytes.
- **`tables/`** — `.tbl` character maps (your own work).
- **`scripts/`** — `.edit` scripts for the `professional-editor` tools.
- **`notes/`** — plain-text working notes (offsets found, strings to change).

## Working with a ROM you own

Keep the ROM outside the repo and point the editor at it:

```sh
# dump a text slot from a local copy you own (not committed):
professional-editor/c/nes-edit dump /path/to/your.nes 1A2C 16 light-load/tables/your.tbl

# apply an edit script to a working copy, originals untouched:
professional-editor/c/nes-edit apply /path/to/your.nes light-load/scripts/your.edit /path/to/your.edited.nes
```

See [`../professional-editor/README.md`](../professional-editor/README.md) and
[`../professional-editor/docs/WORKFLOW.md`](../professional-editor/docs/WORKFLOW.md)
for the full dump → script → apply → verify loop.
