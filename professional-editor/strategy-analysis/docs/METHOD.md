# Strategy Analysis — Method & Honest Limits

This module helps you **study the algorithms and decision logic in an NES ROM
you own**, for research and coursework. It produces *your own structural and
behavioral evidence* about how a game's code is organized and which routines run
after which decisions. It ships no copyrighted ROM content and copies no game
code.

## What "pulling strategy" can and cannot mean

A game's strategy/AI is **behavior that emerges when 6502 machine code runs** —
branches, lookup tables, RNG, and timing distributed across banked code. It is
**not** a labelled block you can locate and extract. Two consequences, stated
plainly so results are never oversold:

- **No universal extractor exists.** Because code locations, bank layouts, and
  internal organization differ per title (different mappers, no shared engine),
  there is no header or convention that marks "the strategy." Any claim of a
  one-click, guaranteed strategy dump across the library is false.
- **Decision-gated code is a reachability problem.** Behavior that only triggers
  after a player/NPC decision lives on code paths reached dynamically. Proving
  you have found *every* such path is equivalent to exhaustively exploring an
  interactive program's reachable states — the halting/state-explosion barrier.
  Static analysis **under-approximates** (misses dynamically-computed targets);
  dynamic analysis **sees only what was exercised**. Neither is complete, and we
  say so in every report.

## The uniform, sound approach

What *is* uniform across any ROM is the **method**, not a magic output:

1. **Static structure (Part A).** From the iNES/mapper layout, follow the 6502
   reset/NMI/IRQ vectors with a recursive-descent + linear sweep pass to
   classify **code vs data**, recover basic blocks, and — crucially — detect
   **jump tables**, the classic structure behind "branch on a decision." This
   gives a map of *where* decision logic plausibly lives.

2. **Dynamic evidence (Part B).** Run the ROM in an emulator you own and record
   an **execution trace**: PC coverage, branch outcomes, and RAM reads/writes,
   each tagged with the **decision context** active at the time (a player/NPC
   choice the harness marks). Ingesting that trace shows which routines and
   tables *actually activate* after each decision — turning "decision-gated
   strategy" into observed, reproducible coverage.

A→B together: static analysis says *where to look*; the trace says *what ran and
when*. The output is a **Strategy Evidence Report** — the raw material for a
written analysis, not a copied algorithm.

## Shared data model (all four implementations)

All four languages (C, C++, SLeeLa, Java) share these schemas so results are
comparable and reproducible.

### Static map (Part A output) — `*.map.tsv`

```
# kind    bank  addr    end     detail
vector    -     FFFC    FFFD    reset=8000
code      0     8000    8012    block
data      0     8013    801F    table?
jumptable 0     9A00    9A10    entries=8 base=9A00
```

### Execution trace (Part B input) — `*.trace.tsv`

A line-oriented log an emulator/harness emits. One event per line:

```
# seq   event    pc     arg          decision
1       exec     8000   -            boot
2       branch   8020   taken=1      boot
3       read     -      addr=0300    menu:ATTACK
4       exec     9A00   -            menu:ATTACK
5       jtbl     9A00   index=2      menu:ATTACK
```

- `decision` is the context the harness marks when the player/NPC made a choice
  (e.g. `menu:ATTACK`, `npc:aggress`, `boot`). This is how decision-gated code
  is attributed.

### Strategy evidence (Part B output) — `*.evidence.md`

A structured report: per decision, which routines/addresses and jump tables were
exercised, with coverage counts and explicit **confidence caveats** (what the
run did and did not exercise).

## Honesty clause (printed in every report)

> This report reflects (A) a static over-/under-approximation of reachable code
> and (B) only the code paths the supplied trace actually exercised. It is
> evidence for a human analysis, not a complete or guaranteed extraction of the
> game's strategy. Absence of evidence is not evidence of absence.
