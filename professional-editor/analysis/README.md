# ROM Analysis Tools

The dependency-free Python 3.10+ tools are read-only with respect to source ROMs.

## Commands

    python3 analysis/nes_inspect.py /path/to/game.nes
    python3 analysis/nes_workbench.py /path/to/game.nes
    python3 analysis/nes_workbench.py game-a.nes game-b.nes --out-dir /path/to/private-analysis
    python3 analysis/nes_workbench.py game.nes --trace emulator-trace.json
    python3 -m unittest analysis/test_nes_inspect.py analysis/test_nes_workbench.py

The workbench outputs index.html (a local browser dashboard), analysis.json, disassembly.txt, analysis-summary.md, the base inspector report, and grayscale PGM/PNG tile sheets when CHR-ROM is present. If trace events include an oam field containing 256 byte values (or a 512-character hexadecimal string), the tool extracts 64 sprite records per snapshot and compares adjacent snapshots for changed sprites. Optional palette (32 bytes) and nametable (960 bytes) fields are also summarized. JSON traces can be an array of event objects or an object with an events array; CSV traces need a header row. Supported useful fields include frame, pc, a, x, y, sp, p, input, address, value, event, and scanline.

## What is implemented

- iNES/NES 2.0 container and region metadata inspection.
- Conservative candidate PRG bank windows for NROM and basic MMC1/UxROM/CNROM/MMC3 layouts.
- Linear-sweep decoding of official 6502 opcodes; unknown/unofficial opcodes stay as one-byte directives.
- Recursive reachability candidate seeded from vectors for NROM; standard NES I/O register address hints for PPU, OAM DMA, APU, and controllers.
- CHR tile counts, unique pattern counts, and exact duplicate-pattern detection.
- Branch/call/jump/store/immediate candidate reports, evidence offsets, confidence labels, and broad gameplay/state hypotheses.
- Aggregation of supplied emulator trace JSON/CSV into hot-PC and memory-write summaries.

## Limits

No input ROM is modified. The tools do not launch an emulator. Mapper state is not emulated, control-flow/code-data boundaries are not proven, and a raw CHR tile sheet is not a complete sprite or screen reconstruction. See ../docs/ROM-ANALYSIS-WORKBENCH.md for next layers and interpretation guidance.
