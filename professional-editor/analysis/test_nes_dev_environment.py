"""Integration tests for the terminal workspace generator."""
import importlib.util, json, tempfile, unittest
from pathlib import Path
HERE = Path(__file__).parent
spec = importlib.util.spec_from_file_location("nes_dev_environment", HERE / "nes_dev_environment.py")
nes_dev_environment = importlib.util.module_from_spec(spec)
import sys
sys.modules[spec.name] = nes_dev_environment
spec.loader.exec_module(nes_dev_environment)

class DevEnvironmentTests(unittest.TestCase):
    def make_rom(self, path):
        header = bytearray(b"NES\x1a" + bytes([1, 1]) + bytes(10))
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
                        "game-logic/memory-map.md","game-logic/action-assessment.md","codes/disassembly/linear-sweep.txt","codes/mapper-analysis.json",
                        "codes/circuit/circuit-analysis.json","codes/circuit/circuit.dot","codes/advanced-analysis.json","game-logic/game-rules.json","game-logic/rules-evaluation.json","game-logic/rules-evaluation.md",
                        "codes/hardware-registers.md","reports/analysis.json","reports/analysis.html"]:
                self.assertTrue((out / rel).is_file(), rel)
            self.assertTrue((out/"game-logic/game-rules.json").is_file())
            self.assertTrue((out/"codes/advanced-analysis.json").is_file())
            self.assertTrue(json.loads((out/"manifest.json").read_text())["input_rom_copied"])
            self.assertTrue((out/"rom-project/source/original.nes").is_file())
            self.assertTrue((out/"rom-project/rom-project.json").is_file())
            self.assertTrue(rom.exists()); self.assertFalse((out/"sample-game.nes").exists())
            self.assertFalse(any(root.glob(".nes-analysis-stage-*")))

    def test_rules_from_existing_workspace_survive_force_regeneration(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); rom=root/"sample.nes"; self.make_rom(rom)
            out=nes_dev_environment.generate(rom, root, "Rules Test", None, False)
            rules_path=out/"game-logic/game-rules.json"
            rules=json.loads(rules_path.read_text(encoding="utf-8"))
            rules["objective"]="Reach the exit"
            rules["actions"]=[{"address":"0x8000","label":"entry","verdict":"required","must_precede":[],"must_follow":[],"max_visits":None}]
            rules_path.write_text(json.dumps(rules), encoding="utf-8")
            regenerated=nes_dev_environment.generate(rom, root, "Rules Test", None, True, rules_path)
            stored=json.loads((regenerated/"game-logic/game-rules.json").read_text(encoding="utf-8"))
            self.assertEqual(stored["objective"], "Reach the exit")
            report=json.loads((regenerated/"game-logic/rules-evaluation.json").read_text(encoding="utf-8"))
            self.assertEqual(report["action_count"], 1)

    def test_no_overwrite_without_force(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); rom=root/"sample.nes"; self.make_rom(rom)
            nes_dev_environment.generate(rom, root, None, None, False)
            with self.assertRaises(FileExistsError):
                nes_dev_environment.generate(rom, root, None, None, False)

if __name__ == "__main__":
    unittest.main()
