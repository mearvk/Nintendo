# Graphics Analysis

Scope: NES PPU tile patterns, palettes, nametables, sprite/OAM records, and future platform-specific graphics formats.

## Implementation requirements

- Identify source container and graphics format before decoding.
- Preserve source bytes and export generated assets to a separate output directory.
- Document tile dimensions, bitplanes, palette indices, transparency, endianness, and address/bank assumptions.
- Include synthetic test vectors with known pixel output and round-trip checks where the format permits lossless encoding.
- Reject truncated data and invalid offsets rather than reading beyond buffers.

A viewer or converter is considered implemented only when its code, tests, supported formats, and limitations are documented. Do not commit extracted copyrighted game graphics without permission.
