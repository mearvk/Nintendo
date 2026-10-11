# Proposed Emulator Trace Schema (Draft)

Status: proposal only; not yet a compatibility promise.

One JSON object per line. Required fields for a normalized instruction event:

- `schema_version`: integer.
- `platform`: stable platform identifier.
- `source_emulator` and `source_version`.
- `sequence`: monotonically increasing integer.
- `timestamp`: integer plus explicit `timestamp_unit`.
- `cpu_address`: address as observed by the emulator.
- `physical_offset`: optional integer only when a reliable bank mapping exists.
- `opcode_bytes`: array of byte values.
- `registers`: optional CPU register snapshot.
- `mapping_context`: optional mapper/bank state.

Consumers must validate types, byte ranges, sequence ordering, and size limits. A CPU address must never be silently treated as a physical ROM offset. Preserve source traces and label missing values rather than inventing them.
