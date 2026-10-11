# Hardware Reference Library

This directory documents hardware for preservation, debugging, and original homebrew development. Keep references, observations, and hypotheses clearly separated.

## Suggested platform coverage

- NES/Famicom: 2A03/6502-family CPU behavior, PPU, APU, address spaces, controller ports, cartridge boards and mapper banking.
- SNES/Super Famicom: 65C816-family CPU, PPU, SPC audio subsystem, DMA/HDMA, cartridge enhancement chips.
- Game Boy / Game Boy Color: CPU variants, memory map, PPU timing, audio channels, cartridge controllers.
- Nintendo 64: VR4300, RCP, RDRAM, cartridge and peripheral interfaces.
- GameCube and Wii: PowerPC-family CPU, graphics/audio subsystems, storage and input.
- Later systems: add only with reliable references and a clear scope.

## Per-platform checklist

Record CPU/graphics/audio components, clock assumptions, memory map, interrupt behavior, DMA, controller protocol, cartridge/media formats, timing constraints, known hardware revisions, reference URLs and access dates. Identify uncertain or reverse-engineered claims.

## Evidence standard

Use primary technical documentation where available and cite sources. Mark observations as **confirmed**, **observed**, or **hypothesis**. Never present linear-sweep disassembly as complete source recovery or as proof of executed control flow.
