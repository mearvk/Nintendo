# runtime — our own NES runtime (original 6502 code)

The on-cartridge code every NES game needs, written from scratch in our own
`asm6502` syntax against the public NES hardware interface. Nothing here is
derived from any existing game or library.

| Module | File | Role |
|---|---|---|
| **reset** | `reset/reset.s` | Power-on handler: disable IRQ/rendering, two vblank waits, clear 2 KiB RAM, jump to `main`. Includes safe NMI/IRQ stubs. |
| **ppu-driver** | `ppu-driver/ppu.s` | Load palette ($3F00), upload VRAM blocks, OAM sprite DMA ($4014), enable rendering. |
| **input** | `input/input.s` | Strobe + shift-read controller 1 into a zero-page button byte; a `check_button` helper. |
| **audio-engine** | `audio-engine/audio.s` | 6502 half of our audio: steps the `NTSND v1` stream (from `tools/audio-builder`) and writes APU registers each frame. |

## How these fit together

```
reset  --->  (main loop you write)
   |            |-- ppu_load_palette / ppu_upload / ppu_oam_dma / ppu_enable
   |            |-- read_pad1 / check_button
   |            |-- audio_init / audio_tick   (call audio_tick once per frame)
```

Assemble each with `../tools/asm6502/asm6502`, then combine them with your main
loop and link a bootable image via `../tools/rom-linker/rom_link` (or the
one-command `../build/build_driver`).

## Register reference used here

- PPU: `$2000` PPUCTRL, `$2001` PPUMASK, `$2002` PPUSTATUS, `$2003` OAMADDR,
  `$2006` PPUADDR, `$2007` PPUDATA, `$4014` OAMDMA.
- Input: `$4016` (pad 1), `$4017` (pad 2).
- Audio: `$4000`–`$4003` (pulse 1), `$4015` (channel enable).

These are public hardware facts we program *to* — not third-party code.

## Note on our assembler subset

`asm6502` implements a growing subset of the 6502 ISA (see
`../tools/asm6502/asm6502.c`). Some modules use only what the subset currently
supports; extend the `ISA[]` table to add instructions (e.g. `SEI`, `PHA`) as
the runtime grows.
