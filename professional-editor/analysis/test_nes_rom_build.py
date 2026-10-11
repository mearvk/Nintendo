import json
import tempfile
import unittest
from pathlib import Path

from analysis import nes_rom_build


def make_rom(path):
    header = bytearray(16)
    header[:4] = b"NES\x1a"
    header[4] = 1
    header[5] = 1
    body = bytearray([0xEA]) * (16384 + 8192)
    body[0x100:0x104] = b"NAME"
    path.write_bytes(header + body)
    return path


class NesRomBuildTests(unittest.TestCase):
    def test_init_and_build_preserves_unpatched_bytes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            rom = make_rom(root / "original.nes")
            original = rom.read_bytes()
            project = root / "project"
            nes_rom_build.init_project(rom, project, "Test Game")
            manifest_path = project / "rom-project.json"
            manifest = json.loads(manifest_path.read_text())
            manifest["patches"] = [{
                "id": "title-name", "enabled": True, "file_offset": "0x110",
                "expected_hex": "4E414D45", "replacement_hex": "47414D45",
                "description": "Change the four-byte title token", "category": "name"
            }]
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            report = nes_rom_build.build_project(manifest_path)
            output = (project / "build/modified.nes").read_bytes()
            self.assertEqual(output[0x110:0x114], b"GAME")
            self.assertEqual(output[:0x110], original[:0x110])
            self.assertEqual(output[0x114:], original[0x114:])
            self.assertEqual(report["patch_count"], 1)
            self.assertTrue(report["validation"]["header_preserved"])
            self.assertNotEqual(report["baseline_sha256"], report["output_sha256"])

    def test_wrong_expected_bytes_refuse_build(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            rom = make_rom(root / "original.nes")
            project = root / "project"
            nes_rom_build.init_project(rom, project)
            manifest_path = project / "rom-project.json"
            manifest = json.loads(manifest_path.read_text())
            manifest["patches"] = [{"id":"stale","enabled":True,"file_offset":"0x110",
                "expected_hex":"00000000","replacement_hex":"11111111"}]
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "expected-byte mismatch"):
                nes_rom_build.build_project(manifest_path)

    def test_overlapping_patches_refuse_build(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            rom = make_rom(root / "original.nes")
            project = root / "project"
            nes_rom_build.init_project(rom, project)
            manifest_path = project / "rom-project.json"
            manifest = json.loads(manifest_path.read_text())
            manifest["patches"] = [
                {"id":"a","enabled":True,"file_offset":"0x110","expected_hex":"4E414D45","replacement_hex":"47414D45"},
                {"id":"b","enabled":True,"file_offset":"0x112","expected_hex":"4D45","replacement_hex":"4F4B"}
            ]
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "overlaps another"):
                nes_rom_build.build_project(manifest_path)

    def test_header_changes_are_blocked_by_default(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            rom = make_rom(root / "original.nes")
            project = root / "project"
            nes_rom_build.init_project(rom, project)
            manifest_path = project / "rom-project.json"
            manifest = json.loads(manifest_path.read_text())
            manifest["patches"] = [{"id":"header","enabled":True,"file_offset":"0x4",
                "expected_hex":"01","replacement_hex":"02"}]
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "header edits are disabled"):
                nes_rom_build.build_project(manifest_path)

    def test_modified_baseline_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            rom = make_rom(root / "original.nes")
            project = root / "project"
            nes_rom_build.init_project(rom, project)
            baseline = project / "source/original.nes"
            data = bytearray(baseline.read_bytes())
            data[0x110] ^= 1
            baseline.write_bytes(data)
            with self.assertRaisesRegex(ValueError, "Baseline SHA-256 mismatch"):
                nes_rom_build.build_project(project / "rom-project.json")

    def test_report_cannot_overwrite_output_rom(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            rom = make_rom(root / "original.nes")
            project = root / "project"
            nes_rom_build.init_project(rom, project)
            with self.assertRaisesRegex(ValueError, "Report path cannot overwrite"):
                nes_rom_build.build_project(project / "rom-project.json",
                    output="build/same.bin", report_path="build/same.bin")

    def test_truncated_rom_rejected(self):
        with self.assertRaisesRegex(ValueError, "ROM is truncated"):
            nes_rom_build.inspect_rom(b"NES\x1a" + bytes([1, 1]) + bytes(10))

    def test_bad_magic_rejected(self):
        with self.assertRaisesRegex(ValueError, "missing NES"):
            nes_rom_build.inspect_rom(b"not a ROM")


if __name__ == "__main__":
    unittest.main()
