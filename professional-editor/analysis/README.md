# ROM Analysis Tools

`nes_inspect.py` is a dependency-free, read-only first-pass inspector for local NES ROM files. It writes a JSON report and a grayscale PGM tile sheet for CHR-ROM. It never writes to the source ROM.

```sh
python3 analysis/nes_inspect.py /path/to/game.nes
python3 analysis/nes_inspect.py game-a.nes game-b.nes --out-dir /path/to/private-analysis
```

Each input gets a separate output directory beneath the selected output root. The report distinguishes declared region sizes from bytes actually available and records warnings for malformed or truncated files. The tile sheet is grayscale because the actual PPU palette and screen composition are not encoded in raw CHR tiles alone.

See the ROM Analysis Workbench document for the planned sprite, mapper-aware disassembly, settings, gameplay, and emulator-trace layers.
