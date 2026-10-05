# NTS structural analysis — NobunagaAmbition002.nes

## merit

| metric | value | note |
|---|---|---|
| format | iNES | container variant |
| prg_banks | 16 | 256 KiB of program ROM |
| chr_banks | 16 | 128 KiB of character ROM |
| file_size | 384 KiB | actual bytes on disk |
| computed_size | 384 KiB | header + trainer + prg + chr |
| header_fingerprint | 0x4E35B637 | FNV-style accumulator over the 16-byte header |

## strategy

| metric | value | note |
|---|---|---|
| mapper | 5 | memory-mapper board number (determines bank-switching surface) |
| mirroring | horizontal | nametable arrangement; drives scroll strategy |
| chr_source | CHR-ROM | tiles fixed in ROM |
| persistence | battery-backed WRAM | progress can be saved |
| bank_switch_surface | multi-bank | code/data paged through the mapper |

## components

| metric | value | note |
|---|---|---|
| header | 0x00000000 +0 KiB | offset 0x00000000, length 16 B |
| prg | 0x00000010 +256 KiB | offset 0x00000010, length 262144 B |
| chr | 0x00040010 +128 KiB | offset 0x00040010, length 131072 B |

## conditions

| condition | satisfied | rationale |
|---|---|---|
| play | yes | valid header, PRG present, regions fit within the image |
| guarantee | no | file size differs from the computed region layout |
| chapters | yes | 16 PRG banks: content is partitioned into addressable chapters |
| win | no | not a terminal configuration (needs mapper + battery + exact size) |
| chemistry | yes | CHR source and bank count are mutually consistent |

