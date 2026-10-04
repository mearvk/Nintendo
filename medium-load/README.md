# medium-load

`medium-load` is the mid-tier **text-only** working area for the
`mearvk/Nintendo` studio. It is the step between `light-load` (quick, single
edits) and `heavy-load` (large campaigns): batch workflows across several
titles, shared character tables, and multi-script edit sets.

> **No ROMs here.** Like every load tier, this folder stores **no** `.nes` /
> `.zip` game images — only references, tables, scripts, and notes. The ROM you
> work on stays local to your machine (you own it) and is never committed.

## What goes in `medium-load`

- **`manifest.tsv`** — a text index of the titles a batch references, by
  filename only (no ROM bytes).
- **`tables/`** — shared `.tbl` character maps reused across several titles.
- **`scripts/`** — `.edit` scripts, typically one per title in the batch.
- **`notes/`** — plain-text working notes (offset maps, string inventories).

## Scale guidance

| Tier | Use it for |
|---|---|
| [`light-load`](../light-load/) | one title, a handful of string/menu edits |
| **`medium-load`** | a batch of titles, shared tables, several scripts |
| [`heavy-load`](../heavy-load/) | large multi-title campaigns, full string tables |

See [`../professional-editor/docs/WORKFLOW.md`](../professional-editor/docs/WORKFLOW.md)
for the dump → script → apply → verify loop each script follows.
