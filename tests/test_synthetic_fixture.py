"""Tests for the original synthetic NES fixture generator."""
from pathlib import Path
import importlib.util
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "fixtures" / "generate_synthetic_nes.py"
SPEC = importlib.util.spec_from_file_location("generate_synthetic_nes", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(MODULE)

class SyntheticNESTests(unittest.TestCase):
    def test_header_and_declared_lengths(self):
        data = MODULE.build_synthetic_rom()
        self.assertEqual(data[:4], b"NES\x1a")
        self.assertEqual(data[4], 1)
        self.assertEqual(data[5], 1)
        self.assertEqual(len(data), 16 + 0x4000 + 0x2000)

    def test_reset_vector(self):
        data = MODULE.build_synthetic_rom()
        prg_start = 16
        self.assertEqual(data[prg_start + 0x3FFC:prg_start + 0x3FFE], b"\x00\x80")

    def test_deterministic_output(self):
        self.assertEqual(MODULE.build_synthetic_rom(), MODULE.build_synthetic_rom())

    def test_cli_writes_separate_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "nested" / "fixture.nes"
            output.parent.mkdir(parents=True)
            output.write_bytes(MODULE.build_synthetic_rom())
            self.assertEqual(output.read_bytes()[:4], b"NES\x1a")

if __name__ == "__main__":
    unittest.main()
