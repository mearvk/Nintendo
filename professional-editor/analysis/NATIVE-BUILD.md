# Native C and C++ ROM tools

The Python workbench remains the feature-complete pipeline. These C99 and C++17 native programs are initial native counterparts for iNES/NES 2.0 header inspection, mapper/PRG/CHR metadata, trainer and mirroring identification, and safe multi-ROM CLI handling. They are separate implementations and do not yet reproduce every Python feature (mapper bank analysis, 6502 disassembly, PNG/PGM tile rendering, trace ingestion, JSON reports, or the HTML dashboard).

## Build

From professional-editor/:

    make
    make test
    make clean

Binaries: build/nes-native-c and build/nes-native-cpp.

CMake alternative:

    cmake -S . -B build-cmake
    cmake --build build-cmake

## Run

    build/nes-native-c /path/to/game.nes
    build/nes-native-cpp --out-dir ./private-analysis /path/to/game.nes game2.nes

Each input gets a Markdown summary under the selected output root. Inputs are opened read-only and are never modified. ROM analysis should only be performed on files you are legally entitled to inspect. Static metadata is not proof of gameplay behavior.
