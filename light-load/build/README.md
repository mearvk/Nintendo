# build — one-command build driver

`build_driver` (C++) orchestrates **our own** tools into a complete, bootable
`.nes`. It shells out only to programs in this repo — no third-party build
system or external dependency.

## Pipeline

```
asm6502 (code)  ─┐
chr_encode (opt) ─┼─► rom_link (header + PRG + CHR + vectors) ─► nts-analyze (validate)
audio_build(opt) ─┘
```

## Build & run

```sh
cd build && make                 # builds build_driver
./build_driver demo/demo.manifest --tools ../tools --nts ../nts/nts-analyze
```

## Manifest format

Simple `key = value` lines (`#` comments):

```
out      = demo.nes          # output image
prg_src  = demo/demo.s       # one or more 6502 sources (repeatable)
chr_src  = tiles.txt         # optional, -> CHR via chr_encode
score    = song.txt          # optional, -> NTSND v1 via audio_build
mapper   = 0
org      = C000              # PRG load / reset org (hex)
battery  = 0
```

## Verified demo

`demo/demo.s` is a self-contained original boot program (reset handler + spin
loop). The driver assembles, links a 16 KiB NROM image with correct interrupt
vectors (`NMI/RESET/IRQ` at `$FFFA–$FFFF`), and validates it. Running the linked
PRG through our `exec-harness` from the reset address reaches the boot marker
(`A=$5A`, zero page `$00=5A`) after the PPU warm-up and RAM clear — proving the
image is genuinely bootable, start to finish, from our own toolchain only.

Build artifacts (`demo.nes`, intermediates) are git-ignored.
