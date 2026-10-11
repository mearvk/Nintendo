# Native C and C++ ROM tools

The Python workbench remains the feature-complete analysis and workspace pipeline. Native C99 and C++17 programs provide header inspection, fixed-length patch building, and a conservative linear-sweep 6502/2A03 disassembly report. The disassembler is not mapper-aware control-flow recovery and does not yet reproduce the circuit/action evidence engine, trace ingestion, full graphics reports, HTML dashboard, or Python's JSON project-manifest workflow.

## Build

From `professional-editor/` on Linux or macOS (with a C compiler, C++17 compiler, Python 3, and Make installed):

    make
    make release-binaries
    make test
    make clean

On **Windows 10/11**, use the PowerShell build script and CMake (Visual Studio 2019+ Build Tools or Visual Studio with the C/C++ workload, plus Python 3):

    powershell -ExecutionPolicy Bypass -File .\\build-windows.ps1
    # or, from this directory:
    cmake -S . -B build-cmake
    cmake --build build-cmake --config Release
    python ..\\scripts\\native_tooling_smoke.py build-cmake\\Release

The PowerShell script configures, builds all C and C++ targets, and runs portable smoke tests. It does not require GNU Make, Bash, or POSIX utilities. On macOS, `./build-macos.sh` runs the Make build and tests; it uses the system Clang toolchain after installing Xcode Command Line Tools (`xcode-select --install`) if needed.

`make test` runs the Python unit tests, native smoke tests (including generated disassembly reports and fixed-length patch checks), and the development-environment CLI help check. The native test target creates a synthetic ROM locally; it does not require a commercial ROM image.

Binaries:
- `build/nes-native-c` — native header inspection.
- `build/nes-native-cpp` — native header inspection.
- `build/nes-rom-build-c` — native fixed-length patch builder.
- `build/nes-rom-build-cpp` — native fixed-length patch builder.
- `build/nes-disasm-c` — native C99 linear-sweep disassembler.
- `build/nes-disasm-cpp` — C++17 build sharing the C-compatible disassembly core.

CMake alternative:

    cmake -S . -B build-cmake
    cmake --build build-cmake --config Release

CMake builds all twelve C/C++ executables. The Makefile's `make test` target runs the combined Python/native smoke suite on POSIX systems. The PowerShell script and Windows CI run the same portable native smoke suite without relying on POSIX shell commands.

## Native linear-sweep disassembly

```sh
build/nes-disasm-c --out-dir ./analysis-output game.nes
build/nes-disasm-cpp --out-dir ./analysis-output game.nes
```

Each command creates a title-specific folder containing `summary.md` and `disassembly.asm`. It reports an iNES/NES 2.0 mapper candidate and walks available PRG bytes using recognized official 6502/2A03 opcodes; unsupported or unofficial opcodes are emitted as `.byte`. This is a linear sweep, not control-flow recovery. Code/data boundaries, mapper bank state, CPU addresses, and relative branch targets are hypotheses that require validation.

## Native round-trip patch command

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
