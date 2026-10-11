<h1 align="center">mearvk · Nintendo Studio</h1>

<p align="center">
  <strong>Original tools for NES ROM editing, analysis, and reproducible research.</strong>
</p>

<p align="center">
  <em>Work on ROM images you are entitled to use. Keep original images and generated assets local.</em>
</p>

---

## Overview

This repository brings together the **Professional Editor**, ROM-analysis workbench, strategy-analysis research tools, SLeeLa model layers, and text-only project workspaces. The editor's byte-changing operations are deliberately conservative: edits are explicit, length-bounded, and written to a separate output file.

The project includes C, C++, Java, and SLeeLa components, but their roles differ. C/C++/Java tools perform byte-oriented operations where implemented; SLeeLa models express validation and strategy. Do not assume every language implements every feature or produces identical artifacts unless a specific parity test says so.

## Main areas

| Area | Purpose |
|---|---|
| [`professional-editor/`](professional-editor/) | Multi-language NES text/menu editor and ROM-analysis workbench |
| [Editor format reference](professional-editor/docs/FORMAT.md) | iNES container layout, text slots, and edit-script grammar |
| [Worked editing workflow](professional-editor/docs/WORKFLOW.md) | Dump → script → apply → verify using synthetic sample data |
| [ROM analysis workbench](professional-editor/docs/ROM-ANALYSIS-WORKBENCH.md) | Static analysis, mapper candidates, graphics metadata, traces, and evidence limits |
| [Native build guide](professional-editor/analysis/NATIVE-BUILD.md) | C99/C++17 tools, Make/CMake, commands, and coverage boundaries |
| [ROM round-trip guide](professional-editor/analysis/ROM-ROUND-TRIP.md) | Baseline manifests and reviewed fixed-length patch builds |
| [Strategy analysis](professional-editor/strategy-analysis/) | Evidence-oriented study of code structure and decision traces |
| [SLeeLa SQL model](lib/sleela-sql/) and [C SQL engine](sleela-sql/) | A SLeeLa validation model alongside a CSV-backed C engine |
| [Light](light-load/), [medium](medium-load/), and [heavy](heavy-load/) load areas | Text-only notes, character tables, scripts, and campaign manifests |

## Professional Editor capabilities

- Inspect iNES/NES 2.0 header metadata and report validation warnings.
- Read and replace fixed-width text/menu slots, optionally using a game-specific `.tbl` character map.
- Apply reviewed, fixed-length byte patches with expected-byte guards.
- Generate a per-title analysis workspace with candidate disassembly, mapper windows, CHR-pattern statistics, control-flow evidence, rules assessments, and optional emulator-trace summaries.
- Create a separate rebuilt ROM from an explicit project manifest and preserve a byte-for-byte baseline for auditability.
- Build native C99 and C++17 header inspectors, fixed-length patch builders, and linear-sweep 6502/2A03 disassemblers.

The native disassemblers are **linear sweeps**, not mapper-aware execution or complete source recovery. Data may be decoded as instructions, and CPU addresses/bank mappings remain hypotheses until validated. The Python workbench has broader analysis and project-manifest features than the current native tools.

## Quick start

From the repository root:

```sh
cd professional-editor

# Build the native tools for the current operating system
make release-binaries

# Run the available test suite
make test

# Inspect a ROM you are entitled to use (read-only)
build/nes-native-c /path/to/game.nes
build/nes-native-cpp /path/to/game.nes

# Produce a conservative native disassembly report
build/nes-disasm-c --out-dir ./private-analysis /path/to/game.nes
build/nes-disasm-cpp --out-dir ./private-analysis /path/to/game.nes

# Generate a Python analysis workspace
python3 analysis/nes_dev_environment.py /path/to/game.nes --title "Research Title"

# Initialize and build a reviewed patch project
python3 analysis/nes_rom_build.py init /path/to/game.nes --project ./projects/research-title --title "Research Title"
python3 analysis/nes_rom_build.py build --project ./projects/research-title/rom-project.json
```

See [examples](professional-editor/examples/README.md) for a synthetic ROM you can generate locally, inspect, and patch without using a commercial game image.

## Native build and CI

The [NES ROM Analysis Native Build workflow](.github/workflows/rom-analysis-native.yml) is configured to build and test on Linux, macOS, and Windows, and to publish OS-specific native-tool artifacts. The workflow configuration is not proof that a particular commit passed: check the Actions run for the exact commit before relying on its result.

The native build currently packages six executables:

- `nes-native-c` and `nes-native-cpp` — header inspection.
- `nes-rom-build-c` and `nes-rom-build-cpp` — reviewed fixed-length patching.
- `nes-disasm-c` and `nes-disasm-cpp` — conservative linear-sweep disassembly.

Build with Make or CMake; see [NATIVE-BUILD.md](professional-editor/analysis/NATIVE-BUILD.md) for details and known limitations.

## Safety and repository hygiene

- Use ROMs you are legally entitled to inspect or modify.
- Do not add new commercial ROMs, saves, patches containing copyrighted material, or extracted copyrighted assets to commits. The repository tree includes pre-existing top-level archive collections outside the editor workbench; their provenance and redistribution rights have not been verified here. Do not assume those archives are cleared for redistribution, and review/remove them through a separate provenance and licensing audit before describing the entire repository as ROM-free.
- Preserve originals; write edits to separate paths.
- Verify expected bytes, bounds, patch overlap, and output reports before using a modified image.
- Treat static analysis, mapper profiles, branch targets, gameplay hypotheses, and trace findings as evidence with explicit uncertainty—not as proof of complete program behavior.
- The load-tier folders are intended for text-only notes and edit artifacts; check each directory's own documentation and ignore rules.

## Project scope

This is a toolkit and research workspace, not a universal NES decompiler, assembler/linker, emulator, or guarantee of game-logic recovery. Length-changing text edits, pointer relocation, mapper-aware code relocation, stateful bank emulation, and fully rendered PPU scenes require additional title-specific or runtime-aware work.

---

<p align="center"><sub>mearvk · Nintendo Studio · original tooling · evidence-first ROM research</sub></p>


## Research toolkit additions

The repository roadmap and subsystem specifications now live in:

- [Development roadmap](DEVELOPMENT-ROADMAP.md) — phased plan and definition of done.
- [Hardware reference library](docs/hardware/README.md) — platform coverage and evidence standards.
- [ROM inventory and provenance](docs/ROM-INVENTORY.md) and [manifest schema](manifests/rom-manifest.schema.json).
- [Project workflow](docs/PROJECT-WORKFLOW.md) and [security guidance](PROJECT-SECURITY.md).
- [Graphics analysis](graphics/README.md) and [graphics implementation status](graphics/FORMAT-STATUS.md).
- [Audio research](audio/README.md) and [audio implementation status](audio/FORMAT-STATUS.md).
- [Controller/input reference](input/README.md) and [draft event schema](input/CONTROLLER-EVENTS.md).
- [Emulator integration](emulation/README.md) and [draft trace schema](emulation/TRACE-SCHEMA.md).
- [Synthetic fixtures](fixtures/README.md), [test strategy](tests/README.md), and [release checklist](releases/README.md).

Generate an original synthetic iNES test image without downloading a game ROM:

```sh
python3 fixtures/generate_synthetic_nes.py /tmp/nintendo-synthetic.nes
python3 -m unittest discover -s tests -p 'test_*.py'
```

These new documents identify planned features as planned; they do not claim that a graphics editor, audio analyzer, emulator, or unified GUI is already implemented. Review [ROM inventory guidance](docs/ROM-INVENTORY.md) before relocating or redistributing any existing binary archives.
