# NES ROM Analysis Workbench

## Run the integrated pipeline

From the professional-editor directory with Python 3.10+:

    python3 analysis/nes_workbench.py /path/to/game.nes
    python3 analysis/nes_workbench.py game-a.nes game-b.nes --out-dir /path/to/private-analysis
    python3 analysis/nes_workbench.py game.nes --trace emulator-trace.json
    python3 -m unittest analysis/test_nes_inspect.py analysis/test_nes_workbench.py

The pipeline combines the read-only container inspector, a conservative 6502/2A03 linear-sweep listing, mapper-specific candidate PRG bank windows, CHR duplicate-pattern statistics, control-flow candidates, gameplay/state hypotheses, and optional emulator-trace aggregation. Outputs include analysis.json, disassembly.txt, analysis-summary.md, and the first-pass report/tile sheet.

## Trace input schema

JSON can be an array of events or an object with an events array. CSV uses column headers. Useful fields include frame, pc, a, x, y, sp, p, input, address, value, event, and scanline. Numeric CPU addresses can be decimal or 0x-prefixed hexadecimal. The trace tool aggregates supplied data; it does not launch an emulator or promise compatibility with every emulator export.

Example JSON:

    [
      {"frame": 10, "pc": "0x8120", "input": "A"},
      {"frame": 10, "pc": "0x8120", "address": "0x0002", "value": "0x01"}
    ]

## Pipeline layers

1. Container validation: iNES/NES 2.0 metadata, trainer, declared/available regions and warnings.
2. PRG bank views: explicit candidate file offsets and CPU address windows. NROM, MMC1, UxROM/CNROM, and MMC3 receive basic bank-window handling; this is not a stateful mapper emulator.
3. 6502 disassembly: official opcodes decode into instruction candidates; unsupported/unofficial opcodes remain one-byte directives so offsets stay aligned. A linear sweep may mistake embedded data for instructions.
4. Graphics pattern analysis: raw CHR tile counts, unique patterns, exact duplicate tile patterns, and a browser-viewable grayscale PNG tile sheet. Optional emulator traces can include 256-byte OAM snapshots to extract 64 sprite records per snapshot (position, tile index, palette, priority, and flips), 32-byte palette snapshots, and 960-byte nametable tile grids; adjacent OAM snapshots are compared for sprite changes. Attribute-table bytes and a fully palette-aware rendered scene still need additional capture/interpretation.
5. Control-flow candidates: branch, call, jump, store and immediate-constant listings, with file offsets and CPU-address candidates.
6. Gameplay hypotheses and traces: low-confidence static candidates plus hot PCs and frequent memory writes from a supplied trace.

## Follow-on work required for a complete workbench

- Stateful mapper-register analysis and more mapper profiles.
- Reachability-based recursive disassembly seeded by reset/NMI/IRQ vectors, with code/data annotations and cross-references.
- Actual palette-aware CHR viewer, nametable/attribute reconstruction, OAM sprite grouping and animation comparison.
- Better RAM/MMIO classification, controller-port detection, game-specific symbol tables and .tbl encodings.
- Emulator integration for frame stepping, watchpoints, input playback, PPU/OAM capture and repeatable scenarios.
- Human-reviewed findings with source evidence and confidence. This pipeline is deliberately a foundation, not a full source decompiler.
