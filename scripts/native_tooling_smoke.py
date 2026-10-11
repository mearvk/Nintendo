#!/usr/bin/env python3
"""Cross-platform smoke tests for every native C/C++ ROM tool (no game ROMs needed)."""
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

BIN = Path(sys.argv[1] if len(sys.argv) > 1 else "build")
EXE = ".exe" if os.name == "nt" else ""

def command(name):
    path = BIN / (name + EXE)
    if not path.exists() and EXE:
        path = BIN / name
    return str(path)

def run_tool(name, *args, expect=0):
    result = subprocess.run([command(name), *map(str, args)], capture_output=True, text=True)
    if result.returncode != expect:
        raise AssertionError(f"{name} returned {result.returncode}, expected {expect}\n{result.stdout}\n{result.stderr}")
    return result

def synthetic_rom(path):
    path.write_bytes(b"NES\x1a" + bytes([1, 1]) + bytes(10) + bytes(16384 + 8192))

class NativeTools(unittest.TestCase):
    def test_core_native_tools(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td); image = root / "synthetic.nes"; synthetic_rom(image)
            out = root / "out"
            for name in ("nes-native-c", "nes-native-cpp"):
                run_tool(name, "--out-dir", out / name, image)
                self.assertTrue((out / name / "synthetic.nes" / "summary.md").is_file())
            for name in ("nes-disasm-c", "nes-disasm-cpp"):
                run_tool(name, "--out-dir", out / name, image)
                self.assertTrue((out / name / "synthetic-native" / "disassembly.asm").is_file())
            for name, value in (("nes-rom-build-c", "01"), ("nes-rom-build-cpp", "02")):
                patched = root / (name + ".nes")
                run_tool(name, "--input", image, "--output", patched, "--patch", "16", "00", value)
                self.assertEqual(patched.stat().st_size, image.stat().st_size)
                self.assertEqual(patched.read_bytes()[16], int(value, 16))
    def test_manifest_hash_and_tamper(self):
        with tempfile.TemporaryDirectory() as td:
            p = Path(td); image = p / "f.nes"; raw = b"NES\x1a" + bytes([1, 1]) + bytes(10) + bytes(16384 + 8192); image.write_bytes(raw)
            for name in ("rom-manifest-c", "rom-manifest-cpp"):
                manifest = p / (name + ".json")
                run_tool(name, "create", image, manifest, "--title", "fixture", "--platform", "NES", "--provenance", "synthetic")
                self.assertEqual(json.loads(manifest.read_text(encoding="utf-8"))["sha256"], hashlib.sha256(raw).hexdigest())
                run_tool(name, "verify", manifest, image)
                image.write_bytes(raw + b"x"); run_tool(name, "verify", manifest, image, expect=1); image.write_bytes(raw)
    def test_chr_export(self):
        with tempfile.TemporaryDirectory() as td:
            p = Path(td); raw = bytearray(b"NES\x1a" + bytes([1, 1]) + bytes(10) + bytes(16384 + 8192)); raw[16 + 16384] = 128
            image = p / "f.nes"; image.write_bytes(raw)
            for name in ("nes-chr-c", "nes-chr-cpp"):
                out = p / (name + ".ppm"); run_tool(name, image, out, "--tiles", "1")
                self.assertTrue(out.read_bytes().startswith(b"P6\n8 8\n255\n"))
    def test_apu_log_validation(self):
        with tempfile.TemporaryDirectory() as td:
            p = Path(td); log = p / "a.csv"; log.write_text("cycle,address,value\n0,0x4000,48\n12,0x4015,3\n", encoding="ascii")
            for name in ("nes-apu-c", "nes-apu-cpp"):
                out = p / (name + ".json"); run_tool(name, log, "--json", out)
                self.assertEqual(json.loads(out.read_text(encoding="utf-8"))["event_count"], 2)

if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]], verbosity=2)
