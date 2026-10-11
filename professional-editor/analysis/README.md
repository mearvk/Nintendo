# ROM Analysis Tools

The dependency-free Python 3.10+ tools are the feature-complete read-only workbench. Native C99 and C++17 starter tools are also available; see NATIVE-BUILD.md.

## Build

From the professional-editor directory:

    make
    make test
    make dev-env-help

Run build/nes-native-c ROM.nes or build/nes-native-cpp ROM.nes for native header inspection. The build also creates build/nes-dev-env, a terminal CLI wrapper; from professional-editor, run build/nes-dev-env /path/to/game.nes. CMake is also supported via cmake -S . -B build-cmake && cmake --build build-cmake. Native tools currently report iNES/NES 2.0 header metadata and mapper/PRG/CHR sizes; they are not yet feature-equivalent to the Python pipeline.

## Generate a per-title terminal workspace

    python3 analysis/nes_dev_environment.py /path/to/game.nes
    python3 analysis/nes_dev_environment.py /path/to/game.nes --title "Super Mario Bros"
    python3 analysis/nes_dev_environment.py /path/to/game.nes --title "My Game" --output-root ~/rom-projects
    python3 analysis/nes_dev_environment.py /path/to/game.nes --trace emulator-trace.json

Equivalent Make target:

    make dev-env ROM=/path/to/game.nes TITLE="Super Mario Bros" OUT=~/rom-projects TRACE=emulator-trace.json

The default output is development-environment-TITLE/ under the current directory. The title can be supplied explicitly or inferred from the ROM filename. Existing output is preserved unless --force is passed to the Python command. The generated workspace includes title description, sprite/tile maps and catalog, gameplay hypotheses, state and memory-map notebooks, disassembly, mapper/register reports, a circuit graph, action assessment, optional emulator traces, JSON/Markdown reports, and a local HTML dashboard. The original ROM is never copied or modified.

See TERMINAL-ENVIRONMENT.md for the full directory layout and interpretation limits. See CIRCUIT-AND-ACTION-ANALYSIS.md for graph semantics and the evidence framework for assessing right action.

## Existing Python commands

    python3 analysis/nes_inspect.py /path/to/game.nes
    python3 analysis/nes_workbench.py /path/to/game.nes
    python3 analysis/nes_workbench.py game-a.nes game-b.nes --out-dir /path/to/private-analysis
    python3 analysis/nes_workbench.py game.nes --trace emulator-trace.json
    python3 -m unittest analysis/test_nes_inspect.py analysis/test_nes_workbench.py analysis/test_nes_circuit.py analysis/test_nes_dev_environment.py

The Python workbench outputs analysis.json, disassembly.txt, analysis-summary.md, a base inspector report, and grayscale PGM/PNG tile sheets when CHR-ROM is present. If trace events include OAM data, it extracts sprite records and compares adjacent snapshots. Palette and nametable fields are also summarized. JSON traces can be an array or an object with an events array; CSV traces need a header row.

## Analysis capabilities

- iNES/NES 2.0 container and region metadata inspection.
- Candidate PRG bank windows for NROM and basic MMC1/UxROM/CNROM/MMC3 layouts.
- Conservative official 6502/2A03 linear disassembly; unknown/unofficial opcodes stay as one-byte directives.
- Recursive NROM reachability candidates, NES hardware-register hints, CHR tile counts and duplicate-pattern detection.
- Branch/call/jump/store/immediate candidates, evidence offsets, confidence labels, and gameplay/state hypotheses.\n- Circuit-style control-flow graph with candidate order, branch paths, loop/repetition signals, paths to return candidates, and evidence-led action assessments.
- Aggregation of supplied emulator trace JSON/CSV into hot-PC and memory-write summaries.

## Limits

No input ROM is modified. The tools do not launch an emulator. Mapper state is approximated, code/data boundaries are not proven, and raw CHR tile sheets are not complete sprite or scene reconstructions. Gameplay interpretations remain hypotheses until supported by runtime evidence.
