# ROM Analysis Tools

The dependency-free Python 3.10+ tools are the feature-complete read-only workbench. Native C99 and C++17 starter tools are also available; see NATIVE-BUILD.md.

## Build

From the professional-editor directory:

```sh
make
make test
make dev-env-help
```

Run `build/nes-native-c ROM.nes` or `build/nes-native-cpp ROM.nes` for native header inspection. The build also creates `build/nes-dev-env`, a terminal CLI wrapper. CMake is also supported via `cmake -S . -B build-cmake && cmake --build build-cmake`. Native tools currently report iNES/NES 2.0 header metadata and mapper/PRG/CHR sizes; they are not yet feature-equivalent to the Python pipeline.

## Generate a per-title terminal workspace

```sh
python3 analysis/nes_dev_environment.py /path/to/game.nes
python3 analysis/nes_dev_environment.py /path/to/game.nes --title "Super Mario Bros"
python3 analysis/nes_dev_environment.py /path/to/game.nes --trace emulator-trace.json
python3 analysis/nes_dev_environment.py /path/to/game.nes --rules game-rules.json --trace emulator-trace.json
```

Equivalent Make target:

```sh
make dev-env ROM=/path/to/game.nes TITLE="Super Mario Bros" OUT=~/rom-projects TRACE=emulator-trace.json
```

The generated workspace includes title description, sprite/tile maps and catalog, gameplay hypotheses, state and memory-map notebooks, disassembly, mapper/register reports, a circuit graph, action assessment, editable game rules, rules evaluation, optional emulator traces, JSON/Markdown reports, and a local HTML dashboard. The original ROM is never copied or modified.

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

## Round-trip ROM edits

Use `python3 analysis/nes_rom_build.py init game.nes --project ./projects/game` to create a baseline and editable `rom-project.json`. Enable reviewed fixed-length byte patches with expected original bytes, then run `python3 analysis/nes_rom_build.py build --project ./projects/game/rom-project.json` to create a separate `build/modified.nes` and audit report. Make targets `rom-project` and `rom-build` are also available. This first build stage does not automatically reconstruct source from disassembly; see `ROM-ROUND-TRIP.md` for safeguards and the roadmap to game-specific text/assembler/linker adapters.

## Current boundaries

The current tools do not launch or control an emulator, single-step the CPU, inject controller inputs, execute mapper register writes, or prove a win/loss condition. Trace-derived replay is an event list, not an emulator command stream. Pixel-accurate scene reconstruction requires synchronized CHR/CHR-RAM, nametable, attribute, palette and OAM data. All game-specific interpretations remain hypotheses until validated.

See `TERMINAL-ENVIRONMENT.md`, `CIRCUIT-AND-ACTION-ANALYSIS.md`, `ADVANCED-ANALYSIS.md`, and `ROM-ROUND-TRIP.md`.
