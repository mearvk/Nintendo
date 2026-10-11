# Terminal ROM Development Environment

The `nes_dev_environment.py` command packages the read-only Python ROM analysis into an organized workspace per title. Requires Python 3.10+ and the standard library.

## Run

    python3 analysis/nes_dev_environment.py /path/to/game.nes
    python3 analysis/nes_dev_environment.py /path/to/game.nes --title "Super Mario Bros"
    python3 analysis/nes_dev_environment.py /path/to/game.nes --title "My Game" --output-root ~/rom-projects
    python3 analysis/nes_dev_environment.py /path/to/game.nes --trace /path/to/emulator-trace.json

By default the output is in the current directory as `development-environment-TITLE/`. Use `--output-root` for a different parent. Existing output is preserved unless `--force` is supplied; force replaces only the matching generated folder under that output root.

## Workspace

- `title-description.md`: editable synopsis and research notes.
- `source-rom/rom-metadata.json`: metadata only; the ROM itself is not copied.
- `sprites/`: grayscale CHR tile maps and a CSV catalog of raw tile patterns.
- `game-logic/`: overview, gameplay hypotheses, state/memory-map notebooks, and evidence-led action assessment.\n- `codes/circuit/`: JSON circuit graph and Graphviz DOT, including sequence edges, branches, loop candidates, and action evidence.
- `codes/disassembly/linear-sweep.txt`: candidate 6502/2A03 disassembly.
- `codes/mapper-analysis.json`: full machine report with approximate mapper findings.
- `codes/hardware-registers.md`: standard hardware-register candidates.
- `traces/`: supplied emulator trace and summary, or instructions for adding one.
- `reports/`: JSON, Markdown, and HTML reports.
- `manifest.json`: input identity, counts, and limitations.

## Circuit and right-action analysis\n\nThe circuit graph treats decoded instructions as nodes and candidate control-flow transitions as edges. It flags sequence/order, conditional branches, calls, backward-edge loop candidates, adjacent repeated instruction runs, and paths toward return-like opcodes. The action assessment compares available precondition/control-flow clues, repeat membership, possible terminal paths, and optional runtime trace support. It intentionally leaves right-action judgments undetermined until game objectives, constraints, side effects, and runtime outcomes are established. See `CIRCUIT-AND-ACTION-ANALYSIS.md`.\n\n## Limits and safety

The ROM is read-only and is not copied into the workspace. Raw CHR patterns are not necessarily complete sprites; palettes, OAM, nametables, and runtime observations are needed for faithful scene reconstruction. Disassembly and mapper mapping are heuristic, so game-logic findings are hypotheses. The program aggregates traces but does not launch an emulator or decompile original source code.
