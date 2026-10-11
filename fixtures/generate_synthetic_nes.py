#!/usr/bin/env python3
"""Generate a tiny, original iNES test image; no commercial ROM data is used."""
from __future__ import annotations
import argparse
from pathlib import Path

HEADER_SIZE = 16

def build_synthetic_rom() -> bytes:
    # 1 x 16 KiB PRG bank, 1 x 8 KiB CHR bank, mapper 0.
    header = bytearray(HEADER_SIZE)
    header[0:4] = b"NES\x1a"
    header[4] = 1
    header[5] = 1
    prg = bytearray([0xEA] * 0x4000)  # NOP-filled synthetic payload
    # Reset vector at the end of the PRG bank points to $8000.
    prg[0x3FFC:0x3FFE] = b"\x00\x80"
    chr_data = bytes(0x2000)
    return bytes(header) + bytes(prg) + chr_data

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="output path for generated fixture")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    data = build_synthetic_rom()
    args.output.write_bytes(data)
    print(f"wrote {len(data)} bytes to {args.output}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
