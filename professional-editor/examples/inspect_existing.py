#!/usr/bin/env python3
"""Analyze an existing NES ROM and create a per-title workspace."""
import argparse
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
ANALYZER = ROOT / "analysis" / "nes_dev_environment.py"


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, help="local NES ROM you are entitled to inspect")
    parser.add_argument("--title", help="human-readable title for the workspace")
    parser.add_argument("--output-root", type=Path, default=Path("workspaces"))
    parser.add_argument("--trace", type=Path, help="optional emulator trace JSON")
    parser.add_argument("--rules", type=Path, help="optional title-specific rules JSON")
    args = parser.parse_args(argv)

    rom = args.rom.expanduser().resolve()
    if not rom.is_file():
        parser.error(f"ROM does not exist: {rom}")
    command = [sys.executable, str(ANALYZER), str(rom),
               "--output-root", str(args.output_root.expanduser().resolve())]
    if args.title:
        command.extend(["--title", args.title])
    if args.trace:
        command.extend(["--trace", str(args.trace.expanduser().resolve())])
    if args.rules:
        command.extend(["--rules", str(args.rules.expanduser().resolve())])
    return subprocess.run(command, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
