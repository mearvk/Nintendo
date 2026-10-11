# NES ROM Examples

These examples cover three different workflows: inspect an existing ROM, make a controlled edit to an existing ROM, and create a small original NES-format ROM from scratch. They use local files and do not bundle commercial ROMs or copyrighted game assets.

Run commands from `professional-editor/`. Python examples require Python 3.10+. Build native tools first with `make` if you want to use the C or C++ patch builder.

## 1. Look at an existing ROM

Use a ROM file you are legally entitled to inspect. The tools analyze the ROM; they do not modify it.

```sh
make
python3 examples/inspect_existing.py /path/to/your-game.nes --title "My Game" --output-root ./workspaces
```

This creates a per-title analysis workspace, including metadata, disassembly and other reports supported by the analyzer. It also creates a round-trip project with a byte-for-byte baseline copy. You can additionally run the native header inspectors:

```sh
build/nes-native-c /path/to/your-game.nes
build/nes-native-cpp /path/to/your-game.nes
```

Use `--help` on the Python script to see all options. Analysis is static unless you separately provide emulator traces; reports may contain hypotheses, not confirmed gameplay behavior.

## 2. Alter an existing ROM safely

The patch builders make explicit, fixed-length byte changes. They do not guess text encodings or relocate pointers. The examples below use a synthetic ROM generated in step 3, so the bytes are known. For a real game, first identify the correct **file offset** and verify the expected original bytes. A CPU address is not automatically a file offset, especially with bank-switched mappers.

### Native C or C++

```sh
make native
build/nes-rom-build-c --input ./build/examples/minimal.nes --output ./build/examples/minimal-c-edit.nes --patch 16 EA 00
build/nes-rom-build-cpp --input ./build/examples/minimal.nes --output ./build/examples/minimal-cpp-edit.nes --patch 16 EA 00
```

Each `--patch` takes `OFFSET EXPECTED_HEX REPLACEMENT_HEX`. The 16-byte iNES header is protected; the replacement must have exactly the same length as the expected bytes. The builder rejects overlapping patches, out-of-bounds offsets, mismatched expected bytes and input/output aliases. It preserves the source file.

### Python project manifest

```sh
python3 analysis/nes_rom_build.py init ./build/examples/minimal.nes --project ./build/examples/minimal-project --title "Minimal Example"
```

Open `build/examples/minimal-project/rom-project.json` and edit the disabled example patch. For instance, change its fields to:

```json
{
  "id": "replace-first-opcode",
  "enabled": true,
  "file_offset": "0x10",
  "expected_hex": "EA",
  "replacement_hex": "00",
  "description": "Replace the first NOP with BRK for demonstration",
  "category": "logic"
}
```

Replace the existing item in the `patches` array with that object, then build:

```sh
python3 analysis/nes_rom_build.py build --project ./build/examples/minimal-project/rom-project.json
```

The modified ROM is written to `build/examples/minimal-project/build/modified.nes`; a JSON report records the edits and structural checks. This demonstrates byte patching, not a meaningful game-logic change. Run the result in an emulator and test it before considering it valid.

## 3. Create a new ROM

Generate a tiny original iNES/NROM-128 example. It contains a reset vector, a short 6502 program that loops forever, and zero-filled CHR-ROM data. It is a teaching fixture, not a complete playable game.

```sh
python3 examples/create_minimal_rom.py --output ./build/examples/minimal.nes
```

Then inspect and patch the newly created file using steps 1 and 2. To build a project directory for it:

```sh
python3 analysis/nes_rom_build.py init ./build/examples/minimal.nes --project ./build/examples/new-rom-project --title "My Original ROM"
python3 analysis/nes_rom_build.py build --project ./build/examples/new-rom-project/rom-project.json
```

The initial project build is a no-op unless you enable a patch in the manifest.

## What “create a new ROM” means here

The generator creates a valid, small NES container with a minimal program and vectors. It does **not** provide a full assembler, linker, graphics pipeline, music engine, mapper configuration system, or game engine. For a real new game, you will need to add program code, graphics/CHR data, reset/interrupt handling, controller input, PPU setup, game logic and tests. Keep generated ROMs and build outputs local; do not commit copyrighted ROM images.

## Quick reference

| Goal | Command |
|---|---|
| Build all tools | `make` |
| Analyze an existing ROM | `python3 examples/inspect_existing.py ROM.nes` |
| Generate a new minimal ROM | `python3 examples/create_minimal_rom.py --output build/examples/minimal.nes` |
| Patch bytes with C | `build/nes-rom-build-c --input IN.nes --output OUT.nes --patch OFFSET EXPECTED REPLACEMENT` |
| Patch bytes with C++ | `build/nes-rom-build-cpp --input IN.nes --output OUT.nes --patch OFFSET EXPECTED REPLACEMENT` |
| Create an editable project | `python3 analysis/nes_rom_build.py init ROM.nes --project PROJECT` |
| Rebuild a project | `python3 analysis/nes_rom_build.py build --project PROJECT/rom-project.json` |
| Run tests | `make test` |
