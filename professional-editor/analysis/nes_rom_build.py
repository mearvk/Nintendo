#!/usr/bin/env python3
"""Conservative, reproducible byte-patch build pipeline for NES ROM projects.

This tool does not pretend that linear disassembly is complete source code. It
applies explicitly reviewed file-offset patches to an immutable baseline ROM.
"""
import argparse
import hashlib
import json
import os
import struct
import sys
import tempfile
from pathlib import Path

SCHEMA_VERSION = "1.0"
MAGIC = b"NES\x1a"


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def parse_hex_bytes(value, field):
    if isinstance(value, bytes):
        return value
    if not isinstance(value, str):
        raise ValueError(f"{field} must be a hexadecimal string")
    cleaned = value.replace("0x", "").replace(" ", "").replace("_", "").replace(",", "")
    if len(cleaned) % 2:
        raise ValueError(f"{field} must contain complete byte pairs")
    try:
        return bytes.fromhex(cleaned)
    except ValueError as exc:
        raise ValueError(f"{field} contains invalid hexadecimal bytes") from exc


def inspect_rom(data):
    if len(data) < 16 or data[:4] != MAGIC:
        raise ValueError("Input is not a valid iNES/NES 2.0 ROM: missing NES\\x1a header")
    h = data[:16]
    nes2 = (h[7] & 0x0C) == 0x08
    mapper = (h[6] >> 4) | (h[7] & 0xF0)
    if nes2:
        mapper |= (h[8] & 0x0F) << 8
    trainer = 512 if h[6] & 0x04 else 0
    if nes2:
        prg_msb = h[9] & 0x0F
        chr_msb = (h[9] >> 4) & 0x0F
        if prg_msb == 0x0F:
            prg_size = (1 << (h[4] >> 2)) * ((h[4] & 3) * 2 + 1)
        else:
            prg_size = ((prg_msb << 8) | h[4]) * 16384
        if chr_msb == 0x0F:
            chr_size = (1 << (h[5] >> 2)) * ((h[5] & 3) * 2 + 1)
        else:
            chr_size = ((chr_msb << 8) | h[5]) * 8192
    else:
        prg_size = h[4] * 16384
        chr_size = h[5] * 8192
    offset = 16 + trainer
    declared_end = offset + prg_size + chr_size
    if declared_end > len(data):
        raise ValueError(f"ROM is truncated: header declares at least {declared_end} bytes, found {len(data)}")
    return {
        "format": "NES 2.0" if nes2 else "iNES",
        "mapper": mapper,
        "trainer_bytes": trainer,
        "prg_rom_bytes": prg_size,
        "chr_rom_bytes": chr_size,
        "header_bytes": 16,
        "prg_file_offset": offset,
        "chr_file_offset": offset + prg_size,
        "declared_content_end": declared_end,
        "file_bytes": len(data),
        "trailing_bytes": max(0, len(data) - declared_end),
        "header_hex": h.hex(),
    }


def empty_project(baseline_path, data, title=None):
    info = inspect_rom(data)
    return {
        "schema_version": SCHEMA_VERSION,
        "project_title": title or Path(baseline_path).stem,
        "baseline": {
            "path": "source/original.nes",
            "sha256": sha256(data),
            "size_bytes": len(data),
            "format": info["format"],
            "mapper": info["mapper"],
        },
        "build": {
            "output": "build/modified.nes",
            "allow_header_changes": False,
            "allow_resize": False,
            "description": "Only reviewed fixed-length byte patches are applied.",
        },
        "patches": [
            {
                "id": "example-disabled",
                "enabled": False,
                "file_offset": "0x000000",
                "expected_hex": "",
                "replacement_hex": "",
                "description": "Replace this example with a reviewed patch.",
                "category": "text|name|graphics|logic|strategy|other",
            }
        ],
        "notes": [
            "Offsets are absolute file offsets, not CPU addresses.",
            "Keep the original ROM unchanged and only distribute ROMs you have rights to modify.",
            "Use a game-specific text codec and verified pointer-table updates for message/name edits.",
            "Do not use a disassembly address as a file offset without a validated mapper/bank mapping.",
        ],
    }


def atomic_write(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=str(path.parent))
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(tmp_name, path)
    except Exception:
        try:
            os.unlink(tmp_name)
        except OSError:
            pass
        raise


def init_project(rom_path, project_dir, title=None, force=False):
    rom_path = Path(rom_path).expanduser().resolve()
    project_dir = Path(project_dir).expanduser().resolve()
    if not rom_path.is_file():
        raise FileNotFoundError(f"ROM not found: {rom_path}")
    data = rom_path.read_bytes()
    info = inspect_rom(data)
    if project_dir.exists() and any(project_dir.iterdir()) and not force:
        raise FileExistsError(f"Project directory is not empty: {project_dir}; use --force to reuse it")
    baseline = project_dir / "source" / "original.nes"
    manifest = project_dir / "rom-project.json"
    if baseline.exists() and sha256(baseline.read_bytes()) != sha256(data) and not force:
        raise FileExistsError("Project already has a different baseline ROM; use a new project directory")
    if not baseline.exists():
        atomic_write(baseline, data)
    if manifest.exists() and not force:
        existing = json.loads(manifest.read_text(encoding="utf-8"))
        if existing.get("baseline", {}).get("sha256") != sha256(data):
            raise ValueError("Existing project manifest does not match this ROM")
    else:
        manifest_data = empty_project(rom_path, data, title)
        manifest.parent.mkdir(parents=True, exist_ok=True)
        manifest.write_text(json.dumps(manifest_data, indent=2) + "\n", encoding="utf-8")
    (project_dir / "build").mkdir(parents=True, exist_ok=True)
    report = {
        "source_path": str(rom_path),
        "project_path": str(project_dir),
        "baseline_sha256": sha256(data),
        "rom": info,
        "manifest": str(manifest),
        "baseline_copy": str(baseline),
        "note": "Baseline copied byte-for-byte. Source ROM was not modified.",
    }
    (project_dir / "rom-project-init.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def load_manifest(path):
    path = Path(path).expanduser().resolve()
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema_version") != SCHEMA_VERSION:
        raise ValueError(f"Unsupported project schema_version: {manifest.get('schema_version')}")
    return path, manifest


def build_project(manifest_path, output=None, report_path=None):
    manifest_path, manifest = load_manifest(manifest_path)
    root = manifest_path.parent
    baseline_cfg = manifest.get("baseline", {})
    baseline_path = (root / baseline_cfg.get("path", "source/original.nes")).resolve()
    try:
        baseline_path.relative_to(root.resolve())
    except ValueError:
        raise ValueError("Baseline path must remain inside the project directory")
    if not baseline_path.is_file():
        raise FileNotFoundError(f"Baseline ROM not found: {baseline_path}")
    original = baseline_path.read_bytes()
    if sha256(original) != baseline_cfg.get("sha256"):
        raise ValueError("Baseline SHA-256 mismatch; refusing to build from modified or wrong source ROM")
    info = inspect_rom(original)
    result = bytearray(original)
    edits = []
    occupied = set()
    for index, patch in enumerate(manifest.get("patches", [])):
        if not patch.get("enabled", False):
            continue
        patch_id = str(patch.get("id") or f"patch-{index + 1}")
        raw_offset = patch.get("file_offset")
        try:
            offset = int(raw_offset, 0) if isinstance(raw_offset, str) else int(raw_offset)
        except (TypeError, ValueError):
            raise ValueError(f"{patch_id}: file_offset must be an integer or 0x-prefixed string")
        expected = parse_hex_bytes(patch.get("expected_hex", ""), f"{patch_id}.expected_hex")
        replacement = parse_hex_bytes(patch.get("replacement_hex", ""), f"{patch_id}.replacement_hex")
        if not expected:
            raise ValueError(f"{patch_id}: expected_hex cannot be empty")
        if len(expected) != len(replacement) and not manifest.get("build", {}).get("allow_resize", False):
            raise ValueError(f"{patch_id}: replacement length differs from expected bytes; resizing is disabled")
        if offset < 16 and not manifest.get("build", {}).get("allow_header_changes", False):
            raise ValueError(f"{patch_id}: header edits are disabled by default")
        end = offset + len(expected)
        if offset < 0 or end > len(original):
            raise ValueError(f"{patch_id}: patch range is outside the baseline ROM")
        if any(pos in occupied for pos in range(offset, end)):
            raise ValueError(f"{patch_id}: patch overlaps another enabled patch")
        actual = bytes(result[offset:end])
        if actual != expected:
            raise ValueError(f"{patch_id}: expected-byte mismatch at file offset 0x{offset:X}; baseline may differ or patches overlap")
        occupied.update(range(offset, end))
        if len(replacement) != len(expected):
            raise ValueError(f"{patch_id}: variable-length edits are not supported by this fixed-length patch builder; update banks/pointers with a future linker workflow")
        result[offset:end] = replacement
        edits.append({
            "id": patch_id, "file_offset": f"0x{offset:X}",
            "length": len(replacement), "expected_hex": expected.hex().upper(),
            "replacement_hex": replacement.hex().upper(),
            "description": patch.get("description", ""),
            "category": patch.get("category", "other"),
        })
    built = bytes(result)
    output_cfg = manifest.get("build", {}).get("output", "build/modified.nes")
    output_path = Path(output).expanduser().resolve() if output else (root / output_cfg).resolve()
    manifest_abs = manifest_path.resolve()
    protected = {baseline_path, manifest_abs, (root / "rom-project-init.json").resolve()}
    try:
        output_path.relative_to(root.resolve())
    except ValueError:
        raise ValueError("Output path must remain inside the project directory")
    if output_path in protected:
        raise ValueError("Output path cannot overwrite the baseline or project metadata")
    out_info = inspect_rom(built)
    if len(built) != len(original):
        raise ValueError("Output size changed unexpectedly")
    if not manifest.get("build", {}).get("allow_header_changes", False) and built[:16] != original[:16]:
        raise ValueError("ROM header changed despite header changes being disabled")
    atomic_write(output_path, built)
    diff_ranges = []
    start = None
    for i, (a, b) in enumerate(zip(original, built)):
        if a != b and start is None:
            start = i
        if a == b and start is not None:
            diff_ranges.append({"start": f"0x{start:X}", "end_exclusive": f"0x{i:X}", "length": i - start})
            start = None
    if start is not None:
        diff_ranges.append({"start": f"0x{start:X}", "end_exclusive": f"0x{len(built):X}", "length": len(built) - start})
    report = {
        "schema_version": SCHEMA_VERSION,
        "project_title": manifest.get("project_title", ""),
        "baseline_sha256": sha256(original),
        "output_sha256": sha256(built),
        "baseline_size_bytes": len(original),
        "output_size_bytes": len(built),
        "rom_metadata": out_info,
        "patch_count": len(edits),
        "patches": edits,
        "changed_ranges": diff_ranges,
        "unmodified_bytes_preserved": sum(1 for a, b in zip(original, built) if a == b),
        "validation": {
            "header_preserved": built[:16] == original[:16],
            "size_preserved": len(built) == len(original),
            "patch_expectations_matched": True,
            "overlapping_patches": False,
            "note": "Structural validation only; run emulator/gameplay regression tests to verify behavior.",
        },
        "output_path": str(output_path),
    }
    report_file = Path(report_path).expanduser().resolve() if report_path else root / "build" / "build-report.json"
    try:
        report_file.relative_to(root.resolve())
    except ValueError:
        raise ValueError("Report path must remain inside the project directory")
    if report_file in protected or report_file == output_path:
        raise ValueError("Report path cannot overwrite the baseline, project metadata, or output ROM")
    report_file.parent.mkdir(parents=True, exist_ok=True)
    report_file.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    init = sub.add_parser("init", help="create a project and immutable baseline copy")
    init.add_argument("rom", type=Path)
    init.add_argument("--project", type=Path, default=Path("nes-rom-project"))
    init.add_argument("--title")
    init.add_argument("--force", action="store_true", help="reuse project directory and replace its manifest")
    build = sub.add_parser("build", help="apply enabled reviewed patches and write a new ROM")
    build.add_argument("--project", type=Path, default=Path("nes-rom-project/rom-project.json"))
    build.add_argument("--output", type=Path, help="output ROM path inside project directory")
    build.add_argument("--report", type=Path, help="build report path inside project directory")
    args = parser.parse_args(argv)
    try:
        if args.command == "init":
            result = init_project(args.rom, args.project, args.title, args.force)
            print(json.dumps(result, indent=2))
        else:
            result = build_project(args.project, args.output, args.report)
            print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"nes-rom-build: error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
