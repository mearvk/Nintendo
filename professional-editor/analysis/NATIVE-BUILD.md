# Native C and C++ ROM tools

The Python workbench remains the feature-complete analysis and workspace pipeline. Native C99 and C++17 programs provide header inspection, fixed-length patch building, and a conservative linear-sweep 6502/2A03 disassembly report. Native parity is being expanded deliberately; these programs do not yet reproduce the circuit/action evidence engine, trace ingestion, full graphics reports, HTML dashboard, or Python's JSON project-manifest workflow.

## Build

From `professional-editor/`:

    make
    make test
    make clean

Binaries:
- `build/nes-native-c` — native header inspection.
- `build/nes-native-cpp` — native header inspection.
- `build/nes-rom-build-c` — native fixed-length patch builder.
- `build/nes-rom-build-cpp` — native fixed-length patch builder.\n- `build/nes-disasm-c` — C99 linear-sweep disassembler.\n- `build/nes-disasm-cpp` — C++17 build of the shared native disassembly core.\n- `build/nes-disasm-c` — native C99 disassembly and header summary.\n- `build/nes-disasm-cpp` — C++17 build of the shared native disassembly core.

CMake alternative:

    cmake -S . -B build-cmake
    cmake --build build-cmake --config Release

## Native disassembly\n\n    build/nes-disasm-c --out-dir ./analysis-output game.nes\n    build/nes-disasm-cpp --out-dir ./analysis-output game.nes\n\nEach creates a title-specific directory containing `summary.md` and `disassembly.asm`. The decoder emits official opcode mnemonics it recognizes and emits unsupported/unofficial bytes as `.byte`. This is a linear sweep over available PRG bytes, not control-flow recovery: code/data boundaries, mapper bank windows, CPU addresses, and branch destinations remain candidates.\n\n## Native disassembly\n\n    build/nes-disasm-c --out-dir ./analysis-output game.nes\n    build/nes-disasm-cpp --out-dir ./analysis-output game.nes\n\nEach writes a title-specific folder containing summary.md and disassembly.asm. This is a conservative linear sweep, not mapper-aware control-flow recovery; CPU addresses and branch targets are candidates, and unknown or unofficial bytes remain .byte directives.\n\n## Native round-trip patch command

Both builders use the same CLI and patch semantics:

    build/nes-rom-build-c --input original.nes --output modified.nes \
      --patch 0x4010 00 FF --patch 0x4020 AABB CCDD

    build/nes-rom-build-cpp --input original.nes --output modified.nes \
      --patch 0x4010 00 FF

Each `--patch` takes a file offset, expected original bytes (hex), and replacement bytes (hex). Offsets can be decimal or `0x` hexadecimal. Expected and replacement data must be valid, non-empty, and exactly the same length. Up to 1024 patches are accepted. The builders reject invalid iNES headers, protected-header edits (offsets below 16), out-of-bounds changes, overlapping patches, expected-byte mismatches, and input/output aliases. They write a separate output file and preserve its length and all unpatched bytes.

These native commands are intentionally a low-level patch interface. They do not parse `rom-project.json`, guess character encodings, relocate text pointers, reassemble 6502 instructions, or relink mapper banks. For the full project manifest and build report, use `python3 analysis/nes_rom_build.py` and see `ROM-ROUND-TRIP.md`.

## Header inspection

    build/nes-native-c /path/to/game.nes
    build/nes-native-cpp --out-dir ./private-analysis /path/to/game.nes game2.nes

Input ROMs are opened read-only. These tools are intended for files you are legally entitled to inspect; do not commit game ROMs or extracted copyrighted assets.
