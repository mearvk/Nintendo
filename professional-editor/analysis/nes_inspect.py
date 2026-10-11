#!/usr/bin/env python3
"""Read-only first-pass NES ROM inspector. Python 3.10+, standard library only."""
from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path

HEADER_SIZE = 16
PRG_UNIT = 16 * 1024
CHR_UNIT = 8 * 1024
TRAINER_SIZE = 512

def nes2_size(lsb: int, msb_nibble: int, unit: int) -> int:
    """Decode NES 2.0 ROM size, including exponent/multiplier encoding."""
    if msb_nibble != 0xF:
        return ((msb_nibble << 8) | lsb) * unit
    exponent = lsb >> 2
    multiplier = (lsb & 0x03) * 2 + 1
    return (1 << exponent) * multiplier

def printable_runs(data: bytes, minimum: int = 4, limit: int = 500) -> list[dict]:
    results = []
    for match in re.finditer(rb"[\x20-\x7e]{%d,}" % minimum, data):
        results.append({
            "offset_relative_to_region": match.start(),
            "length": len(match.group()),
            "text": match.group().decode("ascii", errors="replace"),
            "confidence": "low",
            "note": "Printable ASCII candidate; may be code/data, not game text."
        })
        if len(results) >= limit:
            break
    return results

def decode_chr_tile(chr_data: bytes, offset: int) -> list[list[int]]:
    """Decode one NES 2bpp tile (16 bytes) into 8 rows of palette indices."""
    tile = chr_data[offset:offset + 16]
    if len(tile) != 16:
        raise ValueError("incomplete CHR tile")
    rows = []
    for y in range(8):
        lo, hi = tile[y], tile[y + 8]
        rows.append([((lo >> (7-x)) & 1) | (((hi >> (7-x)) & 1) << 1) for x in range(8)])
    return rows

def write_pgm_tile_sheet(path: Path, chr_data: bytes, tiles_per_row: int = 32) -> int:
    tile_count = len(chr_data) // 16
    rows = (tile_count + tiles_per_row - 1) // tiles_per_row
    width, height = tiles_per_row * 8, rows * 8
    shades = (0, 85, 170, 255)
    pixels = bytearray()
    for tile_y in range(rows):
        for pixel_y in range(8):
            scanline = bytearray()
            for tile_x in range(tiles_per_row):
                index = tile_y * tiles_per_row + tile_x
                if index < tile_count:
                    decoded = decode_chr_tile(chr_data, index * 16)
                    scanline.extend(shades[v] for v in decoded[pixel_y])
                else:
                    scanline.extend([0] * 8)
            pixels.extend(scanline)
    path.write_bytes(f"P5\n{width} {height}\n255\n".encode("ascii") + pixels)
    return tile_count

def inspect(path: Path, out_root: Path) -> dict:
    data = path.read_bytes()
    if len(data) < HEADER_SIZE or data[:4] != b"NES\x1a":
        raise ValueError("not a valid iNES/NES 2.0 file: missing NES\\x1a header")
    h = data[:HEADER_SIZE]
    nes2 = (h[7] & 0x0C) == 0x08
    trainer = bool(h[6] & 0x04)
    mapper = (h[6] >> 4) | (h[7] & 0xF0)
    submapper = None
    if nes2:
        mapper |= (h[8] & 0x0F) << 8
        submapper = h[8] >> 4
        prg_size = nes2_size(h[4], h[9] & 0x0F, PRG_UNIT)
        chr_size = nes2_size(h[5], h[9] >> 4, CHR_UNIT)
    else:
        prg_size, chr_size = h[4] * PRG_UNIT, h[5] * CHR_UNIT

    trainer_offset = HEADER_SIZE if trainer else None
    prg_offset = HEADER_SIZE + (TRAINER_SIZE if trainer else 0)
    chr_offset = prg_offset + prg_size
    declared_end = chr_offset + chr_size
    warnings = []
    if prg_size == 0:
        warnings.append("PRG-ROM size is zero; special handling may be required.")
    if len(data) < prg_offset:
        raise ValueError("truncated file: trainer/header region is incomplete")
    if len(data) < chr_offset:
        warnings.append("File ends before declared PRG-ROM region ends.")
    if len(data) < declared_end:
        warnings.append("File is shorter than declared PRG-ROM + CHR-ROM size.")
    if len(data) > declared_end:
        warnings.append(f"{len(data) - declared_end} trailing byte(s) beyond declared payload.")

    prg = data[prg_offset:min(len(data), chr_offset)]
    chr_data = data[chr_offset:min(len(data), declared_end)] if chr_size else b""
    out_dir = out_root / (path.stem or "rom")
    out_dir.mkdir(parents=True, exist_ok=True)
    tile_sheet = None
    tile_count = 0
    if chr_size and len(chr_data) >= 16:
        tile_sheet = "chr-tiles.pgm"
        tile_count = write_pgm_tile_sheet(out_dir / tile_sheet, chr_data[:len(chr_data)//16*16])

    vectors = {}
    if len(prg) >= 6:
        start = len(prg) - 6
        for label, pos in (("nmi", start), ("reset", start+2), ("irq_brk", start+4)):
            address = prg[pos] | (prg[pos+1] << 8)
            vectors[label] = {"cpu_address": f"0x{address:04X}",
                              "prg_relative_offset": pos, "file_offset": prg_offset+pos,
                              "confidence": "medium",
                              "note": "Common vector placement heuristic; mapper banking can affect interpretation."}

    report = {
        "input": str(path.resolve()), "file_size": len(data),
        "format": "NES 2.0" if nes2 else "iNES (legacy)",
        "header": {
            "magic": "NES\\x1a", "prg_rom_bytes_declared": prg_size,
            "chr_rom_bytes_declared": chr_size, "mapper": mapper, "submapper": submapper,
            "trainer_present": trainer, "trainer_file_offset": trainer_offset,
            "mirroring": "four-screen" if h[6] & 0x08 else ("vertical" if h[6] & 0x01 else "horizontal"),
            "battery_or_persistent_memory": bool(h[6] & 0x02),
            "chr_memory": "CHR-ROM" if chr_size else "CHR-RAM or board-specific",
            "region_bits": (h[12] & 0x03) if nes2 else (h[9] & 0x01)
        },
        "regions": {
            "header": {"file_offset": 0, "size": HEADER_SIZE},
            "trainer": {"file_offset": trainer_offset, "size": TRAINER_SIZE if trainer else 0},
            "prg_rom": {"file_offset": prg_offset, "available_bytes": len(prg), "declared_bytes": prg_size},
            "chr_rom": {"file_offset": chr_offset, "available_bytes": len(chr_data), "declared_bytes": chr_size}
        },
        "vectors": vectors,
        "findings": {"ascii_candidates_in_prg": printable_runs(prg),
                     "chr_tile_count_complete_8x8": tile_count, "chr_tile_sheet": tile_sheet},
        "warnings": warnings,
        "limitations": [
            "ASCII scanning misses games using custom tile encodings; a game-specific .tbl table may be needed.",
            "The grayscale CHR sheet shows raw 2bpp indices, not the actual palette or screen composition.",
            "Tiles are not necessarily complete sprites; runtime code combines tiles, palettes, and positions.",
            "No 6502 disassembly, mapper bank analysis, gameplay inference, or dynamic execution is performed yet.",
            "Vector addresses are heuristic and must be interpreted with mapper and bank layout."
        ]
    }
    (out_dir / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    lines = [f"# First-pass ROM inspection: {path.name}", "",
             f"- Format: {report['format']}", f"- Mapper/submapper: {mapper}/{submapper if submapper is not None else 'n/a'}",
             f"- PRG-ROM: {prg_size:,} declared bytes; {len(prg):,} available",
             f"- CHR-ROM: {chr_size:,} declared bytes; {len(chr_data):,} available",
             f"- Trainer: {'yes' if trainer else 'no'}", f"- Complete CHR tiles: {tile_count:,}",
             f"- Printable ASCII candidates: {len(report['findings']['ascii_candidates_in_prg'])}", "",
             "## Warnings"]
    lines += [f"- {w}" for w in warnings] if warnings else ["- None detected by this first-pass check."]
    lines += ["", "## Interpretation limits"] + [f"- {x}" for x in report["limitations"]]
    lines += ["", "Machine-readable details are in report.json."]
    (out_dir / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    return report

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("roms", nargs="+", type=Path, help="local ROM files; inputs are never modified")
    parser.add_argument("--out-dir", type=Path, default=Path("rom-analysis-output"),
                        help="output root (default: ./rom-analysis-output)")
    args = parser.parse_args()
    failures = 0
    for rom in args.roms:
        try:
            report = inspect(rom, args.out_dir)
            print(f"OK  {rom}: mapper {report['header']['mapper']}, "
                  f"PRG {report['header']['prg_rom_bytes_declared']} B, "
                  f"CHR {report['header']['chr_rom_bytes_declared']} B")
        except (OSError, ValueError) as exc:
            failures += 1
            print(f"ERR {rom}: {exc}", file=sys.stderr)
    return 1 if failures else 0

if __name__ == "__main__":
    raise SystemExit(main())
