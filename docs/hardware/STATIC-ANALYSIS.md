# Hardware-aware static analysis

Classify claims as **Observed** (literal bytes or supplied trace), **Decoded** (documented deterministic decoding), **Inferred** (plausible but unproven), or **Unknown** (unsupported or insufficient evidence).

Keep file offsets, CPU addresses, and PRG bank indices distinct. Account for trainers in payload offsets. Do not infer bank switching unless mapper semantics are implemented. Record mapper ID, iNES/NES 2.0 format, PRG/CHR sizes, and unsupported features.

Linear-sweep 6502/2A03 disassembly is not proof of executable code: data tables and graphics can decode as valid instructions. Indirect jumps, computed addresses, interrupt paths, and mapper transitions remain unresolved without additional evidence. Mapper-aware graph traversal, trace import, and symbol correlation remain planned until implemented and tested.

Use synthetic fixtures for vectors, bank boundaries, malformed headers, and truncation. Trace imports must identify emulator, version, trace format, and supplied symbols.
