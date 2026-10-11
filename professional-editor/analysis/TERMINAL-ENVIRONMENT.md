# Terminal ROM Development Environment

The `nes_dev_environment.py` command packages the read-only Python ROM analysis into an organized workspace per title. Requires Python 3.10+ and the standard library.

## Run

```sh
python3 analysis/nes_dev_environment.py /path/to/game.nes
python3 analysis/nes_dev_environment.py /path/to/game.nes --title "Super Mario Bros"
python3 analysis/nes_dev_environment.py /path/to/game.nes --title "My Game" --output-root ~/rom-projects
python3 analysis/nes_dev_environment.py /path/to/game.nes --trace /path/to/emulator-trace.json
python3 analysis/nes_dev_environment.py /path/to/game.nes --rules ./game-rules.json --trace ./trace.json
```

By default the output is in the current directory as `development-environment-TITLE/`. Use `--output-root` for a different parent. Existing output is preserved unless `--force` is supplied; force replaces only the matching generated folder under that output root.

## Workspace

- `title-description.md`: editable synopsis and research notes.
- `source-rom/rom-metadata.json`: metadata only; the ROM itself is not copied.
- `sprites/`: grayscale CHR tile maps and a CSV catalog of raw tile patterns.
- `game-logic/`: gameplay hypotheses, state/memory-map notebooks, action assessment, editable `game-rules.json`, and rules-evaluation reports.
- `codes/circuit/`: JSON circuit graph and Graphviz DOT, including sequence edges, branches, loop candidates, and action evidence.
- `codes/disassembly/linear-sweep.txt`: candidate 6502/2A03 disassembly.
- `codes/mapper-analysis.json`: full machine report with approximate mapper findings.
- `codes/advanced-analysis.json`: mapper profiles, address-region classifications, normalized trace metadata, and declared-rules evaluation.
- `codes/hardware-registers.md`: standard hardware-register candidates.
- `traces/`: supplied emulator trace and summary, or instructions for adding one.
- `reports/`: JSON, Markdown, and HTML reports.
- `manifest.json`: input identity, counts, and limitations.

## Rules and action evaluation

Edit the generated `game-logic/game-rules.json` to describe the title-specific objective, constraints, required/allowed/forbidden actions, and any `must_precede`/`must_follow` address requirements. Supply that file on a later run with `--rules`. The report uses `consistent-with-declared-rule`, `conflicts-with-declared-rule`, and `unresolved`. These are evidence checks against the declaration, not proof of complete gameplay semantics. Static presence is not proof of execution, and a trace only covers the states it recorded.

## Circuit and right-action analysis

The circuit graph treats decoded instructions as nodes and candidate control-flow transitions as edges. It flags sequence/order, conditional branches, calls, backward-edge loop candidates, adjacent repeated instruction runs, and paths toward return-like opcodes. The action assessment compares available control-flow clues, repeat membership, possible terminal paths, and runtime trace support. It intentionally leaves right-action judgments undetermined until game objectives, constraints, side effects, and runtime outcomes are established. See `CIRCUIT-AND-ACTION-ANALYSIS.md`.

## Limits

The ROM is read-only and is not copied into the workspace. Raw CHR patterns are not necessarily complete sprites; palettes, OAM, nametables, attributes, and runtime observations are needed for faithful scene reconstruction. The current pipeline normalizes supplied traces but does not launch or control an emulator, single-step the CPU, or inject controller input. Mapper profiles are descriptive and do not execute bank-switch registers. Disassembly and mapper mapping are heuristic, so game-logic findings remain hypotheses until validated. See `ADVANCED-ANALYSIS.md` for the next-stage workflow.
