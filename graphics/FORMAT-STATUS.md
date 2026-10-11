# Graphics format status

| Capability | Status | Notes |
|---|---|---|
| NES CHR 2bpp tile atlas | Implemented | nes_chr_viewer.py; dependency-free grayscale PPM |
| Palette-accurate rendering | Planned | Requires palette and nametable context |
| Nametable viewer | Planned | Attribute-table and mirroring support needed |
| Sprite/OAM viewer | Planned | Needs documented OAM or trace input |

The viewer supports conventional linear iNES/NES 2.0 CHR sizing, rejects exponent-sized ROMs and CHR-RAM-only images, validates bounds, and never modifies its input.
