# ROM Analysis Tools

The dependency-free Python 3.10+ tools provide the feature-complete analysis, circuit/action evidence, trace reporting, per-title workspace generation, and JSON manifest round-trip workflow. Native C99 and C++17 counterparts are included for header inspection and fixed-length ROM patching; see [NATIVE-BUILD.md](NATIVE-BUILD.md) for the current native coverage boundary.

## Build

From the professional-editor directory:

```sh
make
make test
make dev-env-help
```

Run `build/nes-native-c ROM.nes` or `build/nes-native-cpp ROM.nes` for native header inspection. Native patch builders are `build/nes-rom-build-c` and `build/nes-rom-build-cpp`. CMake is also supported via `cmake -S . -B build-cmake && cmake --build build-cmake`.

## Examples

See [`examples/README.md`](../examples/README.md) for complete workflows to inspect an existing ROM, apply a fixed-length byte patch to a copy, and generate a new minimal iNES/NROM teaching ROM. Quick start:

```sh
make example-rom
make example-inspect ROM=/path/to/your-game.nes TITLE="My Game"
```

## Native fixed-length edits

```sh
build/nes-rom-build-c --input original.nes --output modified.nes --patch 0x4010 00 FF
build/nes-rom-build-cpp --input original.nes --output modified.nes --patch 0x4010 00 FF
```

Each patch is `OFFSET EXPECTED_HEX REPLACEMENT_HEX`. Expected bytes must match the original, replacement length must match, ranges cannot overlap or extend beyond the ROM, and the 16-byte header is protected. The input and output must be different files. These native tools do not yet parse the Python JSON project manifest or provide pointer relocation / source assembly.

## Python workspace and project workflow

```sh
python3 analysis/nes_dev_environment.py /path/to/game.nes --title "Super Mario Bros"
python3 analysis/nes_rom_build.py init game.nes --project ./projects/game
python3 analysis/nes_rom_build.py build --project ./projects/game/rom-project.json
```

The generated workspace includes title description, sprite/tile maps and catalog, gameplay hypotheses, state and memory-map notebooks, disassembly, mapper/register reports, a circuit graph, action assessment, editable game rules, rules evaluation, optional emulator traces, JSON/Markdown reports, and a local HTML dashboard. The input ROM is never modified; each generated workspace includes a byte-for-byte baseline copy under `rom-project/source/original.nes` and a patch manifest so reviewed edits can be rebuilt into a separate ROM.

## Analysis capabilities

- iNES/NES 2.0 container and region metadata inspection.
- Candidate PRG bank windows for common mapper families; advanced profiles document NROM, MMC1, UxROM and CNROM mapping constraints.
- Conservative official 6502/2A03 linear disassembly; unknown/unofficial opcodes stay as one-byte directives.
- Recursive NROM reachability candidates, NES hardware-register hints, CHR tile counts and duplicate-pattern detection.
- Branch/call/jump/store/immediate candidates, evidence offsets, confidence labels, and gameplay/state hypotheses.
- Circuit-style control-flow graph with candidate order, branch paths, loop/repetition signals, paths to return candidates, and evidence-led action assessments.
- Editable title-specific rules, trace-aware order checks, and explicit unresolved/conflict/consistent statuses.
- Candidate CPU RAM, PPU/APU/I/O, expansion, save-RAM, and mapper-controlled address-region classification.
- Normalized JSON trace events and trace-derived input event lists.
- PPU/OAM/palette/nametable snapshot summaries where those fields exist in the trace.

## Current boundaries

The native executables do not yet reproduce every Python workbench feature. Neither implementation automatically reconstructs source from disassembly, relocates pointer tables, or links changed-length strings or code. The Python byte-patch builder also requires explicit reviewed patches and is not a general source-to-ROM assembler/linker.

The tools do not launch or control an emulator, single-step the CPU, inject controller inputs, execute mapper register writes, or prove a win/loss condition. Trace-derived replay is an event list, not an emulator command stream. Pixel-accurate scene reconstruction requires synchronized CHR/CHR-RAM, nametable, attribute, palette and OAM data. All game-specific interpretations remain hypotheses until validated.

See `TERMINAL-ENVIRONMENT.md`, `CIRCUIT-AND-ACTION-ANALYSIS.md`, `ADVANCED-ANALYSIS.md`, and `ROM-ROUND-TRIP.md`.
