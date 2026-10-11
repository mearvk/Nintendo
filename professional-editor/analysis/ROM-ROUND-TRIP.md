# NES ROM Round-Trip Editing and Rebuild

The ROM builder provides a conservative first round-trip loop for the terminal workbench: preserve a byte-for-byte baseline, declare reviewed fixed-length patches, rebuild a separate ROM, and produce a machine-readable audit report.

## Initialize a project

From `professional-editor/`:

```sh
python3 analysis/nes_rom_build.py init /path/to/original.nes --project ./projects/my-game --title "My Game"
```

This creates:

- `source/original.nes`: a byte-for-byte baseline copy.
- `rom-project.json`: editable build manifest and patch list.
- `rom-project-init.json`: source hash and parsed header metadata.
- `build/`: output location for rebuilt ROM and report.

The source ROM is not modified. Existing non-empty project folders are not reused by default.

## Declare a fixed-length patch

Open `rom-project.json` and replace the disabled example patch. For example, if inspection has verified that four bytes at absolute file offset `0x4110` are ASCII `NAME`, change them to `GAME`:

```json
{
  "id": "change-name",
  "enabled": true,
  "file_offset": "0x4110",
  "expected_hex": "4E414D45",
  "replacement_hex": "47414D45",
  "description": "Change verified four-byte name token",
  "category": "name"
}
```

Use the actual offset and bytes from your ROM. This is an illustrative example, not a claim that any particular game stores its title at that offset. The expected-byte check prevents the patch from silently applying to a different revision or stale location. Overlapping patches are rejected.

Supported categories include `text`, `name`, `graphics`, `logic`, `strategy`, and `other`. Categories are documentation metadata; they do not automatically discover text or rewrite code.

## Build a separate ROM

```sh
python3 analysis/nes_rom_build.py build --project ./projects/my-game/rom-project.json
```

Or use Make:

```sh
make rom-project ROM=/path/to/original.nes PROJECT=./projects/my-game TITLE="My Game"
# Edit projects/my-game/rom-project.json to enable and configure patches.
make rom-build PROJECT=./projects/my-game/rom-project.json
```

Default output: `build/modified.nes` inside the project directory. The output may be selected with `--output` or Make's `OUTPUT=`, but it must remain inside the project directory and cannot overwrite the baseline. A `build/build-report.json` records the input/output SHA-256, patch list, changed ranges, header/size checks, and validation limitations.

## Safety guarantees and current scope

- Baseline SHA-256 is checked before every build.
- Every enabled patch must match its expected original bytes.
- Patch ranges must be in bounds and cannot overlap.
- Fixed-length replacement is required; ROM resizing and bank growth are not implemented.
- Header edits are blocked by default.
- Output and report paths are restricted to the project directory.
- Untouched bytes are copied exactly from the baseline.
- Output is written atomically to a separate path.

This first compiler is a deterministic byte-patch builder, not yet a full disassembler-to-source assembler/linker. It does not automatically turn an arbitrary instruction listing back into source. NES games often store text in custom encodings, compressed tables, or shared pointer tables; changing message length can require repacking a bank, updating pointers, and preserving mapper constraints. Code/strategy changes require verified instruction boundaries, free space, branch/call range checks, and often bank-aware relocation. Those should be added as game-specific adapters or a future assembler/linker stage rather than guessed.

## Recommended game-specific edit adapters

For reliable higher-level edits, add title profiles containing:

1. Text codec and dictionary/token table.
2. Message records and their pointer-table entries.
3. CPU-address-to-physical-file-offset mapping, including mapper and selected bank.
4. Known PRG/CHR bank allocation and free-space map.
5. Assembler dialect, reset/vector locations, branch range checks, and relocation rules.
6. Test scenarios and emulator traces for title/menu, normal play, transitions, and failure/win states.

The profile can then expose semantic edits (e.g. replace a dialogue line or change a strategy constant) while compiling them to reviewed patches or assembled bank updates. Unknown mappings remain unresolved and block unsafe writes.
