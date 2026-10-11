"""Integration tests for the terminal workspace generator."""
import json, tempfile, unittest
from pathlib import Path
import nes_dev_environment

class DevEnvironmentTests(unittest.TestCase):
    def make_rom(self, path):
        header = bytearray(b"NES\\x1a" + bytes([1, 1]) + bytes(10))
        prg = bytearray([0xEA] * 16384)
        prg[-6:] = bytes([0x00, 0x80, 0x00, 0x80, 0x00, 0x80])
        path.write_bytes(header + prg + bytes(8192))

    def test_workspace(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); rom = root / "sample-game.nes"; self.make_rom(rom)
            out = nes_dev_environment.generate(rom, root, "Sample Game!", None, False)
            self.assertEqual(out.name, "development-environment-Sample-Game")
            for rel in ["README.md","title-description.md","manifest.json","source-rom/rom-metadata.json",
                        "sprites/sprite-map.png","sprites/background-tiles.png","sprites/sprite-catalog.csv",
                        "game-logic/overview.md","game-logic/gameplay-hypotheses.md","game-logic/state-and-events.md",
                        "game-logic/memory-map.md","codes/disassembly/linear-sweep.txt","codes/mapper-analysis.json",
                        "codes/hardware-registers.md","reports/analysis.json","reports/analysis.html"]:
                self.assertTrue((out / rel).is_file(), rel)
            self.assertFalse(json.loads((out/"manifest.json").read_text())["input_rom_copied"])
            self.assertTrue(rom.exists()); self.assertFalse((out/"sample-game.nes").exists())
            self.assertFalse(any(root.glob(".nes-analysis-stage-*")))

    def test_no_overwrite_without_force(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); rom=root/"sample.nes"; self.make_rom(rom)
            nes_dev_environment.generate(rom, root, None, None, False)
            with self.assertRaises(FileExistsError):
                nes_dev_environment.generate(rom, root, None, None, False)

if __name__ == "__main__":
    unittest.main()
