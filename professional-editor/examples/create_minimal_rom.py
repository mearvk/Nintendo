#!/usr/bin/env python3
"""Generate a tiny original iNES/NROM-128 ROM fixture for the tutorials."""
import argparse
from pathlib import Path


def build_minimal_rom() -> bytes:
    # 16-byte iNES header: one 16 KiB PRG bank, one 8 KiB CHR bank, mapper 0.
    header = bytearray(b"NES\x1a" + bytes(12))
    header[4] = 1
    header[5] = 1

    # NROM-128 PRG is mirrored at $C000-$FFFF. Start at $8000.
    # $8000: NOP; $8001: JMP $8001 (safe intentional infinite loop).
    prg = bytearray([0xEA, 0x4C, 0x01, 0x80])
    prg.extend(bytes(0x4000 - len(prg)))
    # NMI, RESET and IRQ/BRK vectors point to $8000 for this minimal fixture.
    prg[0x3FFA:0x4000] = bytes((0x00, 0x80, 0x00, 0x80, 0x00, 0x80))
    chr_rom = bytes(0x2000)
    return bytes(header) + bytes(prg) + chr_rom


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("build/examples/minimal.nes"))
    args = parser.parse_args(argv)
    output = args.output.expanduser().resolve()
    data = build_minimal_rom()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    print(f"Created {output} ({len(data)} bytes; iNES, mapper 0, 16 KiB PRG, 8 KiB CHR)")
    print("This is a minimal teaching fixture, not a playable game.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
