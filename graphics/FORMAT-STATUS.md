# Graphics Format Status

This file tracks scope and implementation truth. Update it when actual code and tests land.

| Format / feature | Status | Evidence required to mark implemented |
|---|---|---|
| NES 2-bitplane 8×8 pattern decode | Planned | Decoder, known synthetic vector, pixel-level expected output |
| Palette visualization | Planned | Palette rules, test vectors, invalid-index behavior |
| Nametable layout | Planned | Attribute-table decoding tests and documented assumptions |
| Sprite/OAM inspection | Planned | Record parser, bounds tests, coordinate/attribute documentation |
| Lossless encode/decode round trip | Planned | Byte or pixel parity tests with documented caveats |

Do not label a feature implemented based solely on a design note or directory existing.
