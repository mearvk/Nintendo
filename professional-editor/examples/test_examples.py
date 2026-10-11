import importlib.util
import tempfile
import unittest
from pathlib import Path

EXAMPLES = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("create_minimal_rom", EXAMPLES / "create_minimal_rom.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class MinimalRomExampleTests(unittest.TestCase):
    def test_minimal_rom_has_expected_iNES_layout(self):
        data = module.build_minimal_rom()
        self.assertEqual(len(data), 16 + 0x4000 + 0x2000)
        self.assertEqual(data[:4], b"NES\x1a")
        self.assertEqual(data[4], 1)
        self.assertEqual(data[5], 1)
        self.assertEqual(data[6] >> 4, 0)
        self.assertEqual(data[16:20], bytes((0xEA, 0x4C, 0x01, 0x80)))
        self.assertEqual(data[16 + 0x3FFA:16 + 0x4000],
                         bytes((0x00, 0x80, 0x00, 0x80, 0x00, 0x80)))

    def test_output_writes_exact_generated_rom(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / "nested" / "minimal.nes"
            target.parent.mkdir(parents=True)
            target.write_bytes(module.build_minimal_rom())
            self.assertEqual(target.read_bytes(), module.build_minimal_rom())


if __name__ == "__main__":
    unittest.main()
