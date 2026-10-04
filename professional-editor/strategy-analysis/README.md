# Strategy Analysis

Tooling to **study the algorithms and decision logic in an NES ROM you own**,
for research and coursework — in **C, C++, SLeeLa, and Java**. Part of the
`mearvk/Nintendo` Professional Editor.

> **Read [`docs/METHOD.md`](docs/METHOD.md) first.** There is **no universal
> extractor** that can "pull the strategy" from any ROM: a game's AI is behavior
> that emerges when 6502 code runs, not a labelled block, and decision-gated
> code is a reachability problem (the halting/state-explosion barrier). This
> module gives you the *uniform, sound method* — static structure + dynamic
> evidence — and is honest in every report about what it can and cannot prove.
> It ships no copyrighted ROM content and copies no game code.

## The two parts

### Part A — static structure
From the iNES/mapper layout, follow the 6502 reset/NMI/IRQ vectors with
recursive-descent code marking, classify **code vs data**, and detect **jump
tables** (the classic decision-dispatch structure), seeding the code pass from
table entries so indirectly-dispatched routines are found. Output: a `*.map.tsv`.

### Part B — dynamic evidence
Ingest an **execution trace** (PC coverage, branches, RAM reads/writes) that an
emulator/harness emits while *you* run the ROM, each event tagged with the
**decision context** active at the time. Output: a `*.evidence.md` report
attributing exercised routines and jump-table dispatches to the decision that
unlocked them — turning "decision-gated strategy" into observed coverage.

A→B: static says *where to look*; the trace says *what ran and when*.

## Implementations

| Language | Location | Tool |
|---|---|---|
| **C** | [`c/`](c/) | `nes-strat` + `libnesstrat.a` — reference analyzer |
| **C++** | [`cpp/`](cpp/) | `nes-strat-cpp` + `libnesstrat_cpp.a` — C++17 |
| **Java** | [`java/`](java/) | `NesStratCli` + `StaticMap`/`Evidence` — JDK-only, Java 21 |
| **SLeeLa** | [`sleela/`](sleela/) | `StrategyModel.sleela` — confidence/strategy model |

The C, C++, and Java tools produce **byte-identical** maps and reports for the
same inputs (verified). SLeeLa expresses the confidence model over the
per-decision coverage the byte tools produce (its VM has no raw-byte file I/O or
disassembler).

## Usage

```sh
# Part A: static code/data + jump-table map of a ROM you own
c/nes-strat static your.nes your.map.tsv

# Part B: strategy evidence from an emulator trace (cross-ref the static map)
c/nes-strat evidence your.trace.tsv your.evidence.md your.nes

# A then B in one step
c/nes-strat full your.nes your.trace.tsv your-out
```

The C++ (`nes-strat-cpp`) and Java (`NesStratCli`) tools take identical
arguments. See [`docs/METHOD.md`](docs/METHOD.md) for the trace/map/report
schemas and the honesty clause, and [`samples/`](samples/) for a synthetic
worked example (`strat-sample.nes` + `strat-sample.trace.tsv`).

## For college work

The sound, citable workflow: run the ROM you own in an emulator, capture a
decision-tagged trace, generate the evidence report, then **write your own
analysis** of the algorithm from that evidence. The tools produce *your*
structural and behavioral evidence — they do not copy or redistribute the game's
code, and they never claim a complete extraction.
