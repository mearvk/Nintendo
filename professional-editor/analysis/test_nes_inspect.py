import importlib.util
import tempfile
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).with_name("nes_inspect.py")
SPEC = importlib.util.spec_from_file_location("nes_inspect", MODULE_PATH)
nes_inspect = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(nes_inspect)


def synthetic_rom(prg_banks=1, chr_banks=1, trainer=False):
    header = bytearray(16)
    header[:4] = b"NES\x1a"
    header[4] = prg_banks
    header[5] = chr_banks
    if trainer:
        header[6] |= 0x04
    trainer_bytes = bytes([0xAA]) * 512 if trainer else b""
    prg = bytearray(prg_banks * 16384)
    # Place typical NMI/reset/IRQ vectors in the final six bytes.
    prg[-6:] = bytes([0x00, 0x80, 0x00, 0xC0, 0x00, 0xE0])
    chr_data = bytes(chr_banks * 8192)
    return bytes(header) + trainer_bytes + bytes(prg) + chr_data


class InspectorTests(unittest.TestCase):
    def test_valid_ines_and_chr_tile_sheet(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            rom = root / "sample.nes"
            rom.write_bytes(synthetic_rom())
            report = nes_inspect.inspect(rom, root / "out")
            self.assertEqual(report["header"]["mapper"], 0)
            self.assertEqual(report["header"]["prg_rom_bytes_declared"], 16384)
            self.assertEqual(report["header"]["chr_rom_bytes_declared"], 8192)
            self.assertEqual(report["findings"]["chr_tile_count_complete_8x8"], 512)
            self.assertTrue((root / "out" / "sample" / "chr-tiles.pgm").exists())

    def test_trainer_shifts_prg_file_offset(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            rom = root / "trainer.nes"
            rom.write_bytes(synthetic_rom(trainer=True))
            report = nes_inspect.inspect(rom, root / "out")
            self.assertEqual(report["regions"]["prg_rom"]["file_offset"], 16 + 512)
            self.assertTrue(report["header"]["trainer_present"])

    def test_chr_ram_has_no_tile_sheet(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            rom = root / "chrram.nes"
            rom.write_bytes(synthetic_rom(chr_banks=0))
            report = nes_inspect.inspect(rom, root / "out")
            self.assertEqual(report["header"]["chr_memory"], "CHR-RAM or board-specific")
            self.assertIsNone(report["findings"]["chr_tile_sheet"])

    def test_invalid_magic_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            rom = root / "bad.nes"
            rom.write_bytes(b"not an NES rom")
            with self.assertRaises(ValueError):
                nes_inspect.inspect(rom, root / "out")

    def test_tile_plane_decode(self):
        # Low plane sets leftmost pixel; high plane sets rightmost pixel.
        tile = bytearray(16)
        tile[0] = 0x80
        tile[8] = 0x01
        rows = nes_inspect.decode_chr_tile(bytes(tile), 0)
        self.assertEqual(rows[0][0], 1)
        self.assertEqual(rows[0][7], 2)
        self.assertEqual(rows[1], [0] * 8)


if __name__ == "__main__":
    unittest.main()
