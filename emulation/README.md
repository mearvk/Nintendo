# Emulator Integration

This directory is for adapters to external emulators and normalized trace import. It is not itself an emulator.

## Adapter contract

An adapter should document emulator name/version, trace format/version, platform and mapper information, timestamp units, CPU address versus physical file offset, bank mapping, register snapshots, and any lossy conversions. Preserve the original trace and record its SHA-256 hash.

## Safety and correctness

- Do not launch downloaded executables automatically.
- Treat trace files and symbols as untrusted input.
- Bound input sizes and reject malformed records with actionable diagnostics.
- Keep static-analysis results distinct from runtime observations.
- Never infer that an instruction executed merely because a linear disassembler decoded it.

## Planned work

1. Define a versioned JSON Lines trace schema.
2. Add parsers with synthetic fixtures.
3. Integrate one emulator at a time behind a documented adapter.
4. Add replay/correlation tests only when timestamps and bank mappings can be interpreted reliably.
