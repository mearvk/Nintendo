# heavy-load

`heavy-load` is the large-campaign **text-only** working area for the
`mearvk/Nintendo` studio. It is for the biggest jobs: full string-table
inventories, multi-title translation/edit campaigns, and long-running
offset maps that span many sessions.

> **No ROMs here.** Like every load tier, this folder stores **no** `.nes` /
> `.zip` game images — only references, tables, scripts, and notes. The ROM you
> work on stays local to your machine (you own it) and is never committed.

## What goes in `heavy-load`

- **`manifest.tsv`** — a text index of every title in the campaign, by filename
  only (no ROM bytes).
- **`tables/`** — complete `.tbl` character maps, including per-title variants.
- **`scripts/`** — the full set of `.edit` scripts, organized by title.
- **`notes/`** — the master offset maps and string inventories.

## Scale guidance

| Tier | Use it for |
|---|---|
| [`light-load`](../light-load/) | one title, a handful of string/menu edits |
| [`medium-load`](../medium-load/) | a batch of titles, shared tables, several scripts |
| **`heavy-load`** | large multi-title campaigns, full string tables |

Keep the scripts small and verifiable — the editor's slot invariant means every
rename stays within its original slot, so even a large campaign never breaks a
ROM's structure. See
[`../professional-editor/docs/WORKFLOW.md`](../professional-editor/docs/WORKFLOW.md).
