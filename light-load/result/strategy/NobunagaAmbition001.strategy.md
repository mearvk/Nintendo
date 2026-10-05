## strategy

| metric | value | note |
|---|---|---|
| mapper | 1 | memory-mapper board number (determines bank-switching surface) |
| mirroring | horizontal | nametable arrangement; drives scroll strategy |
| chr_source | CHR-RAM | tiles streamed into RAM at runtime |
| persistence | battery-backed WRAM | progress can be saved |
| bank_switch_surface | multi-bank | code/data paged through the mapper |

