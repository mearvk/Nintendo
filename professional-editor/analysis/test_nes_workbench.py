import importlib.util
import tempfile
import unittest
from pathlib import Path
HERE = Path(__file__).parent
def load(name, filename):
    spec=importlib.util.spec_from_file_location(name,HERE/filename)
    module=importlib.util.module_from_spec(spec)
    import sys
    sys.modules[name]=module
    spec.loader.exec_module(module)
    return module
disasm=load("nes_disasm","nes_disasm.py")
workbench=load("nes_workbench","nes_workbench.py")
class DisassemblyTests(unittest.TestCase):
    def test_immediate_and_branch_targets(self):
        r=disasm.disassemble(bytes([0xA9,0x01,0xD0,0x02,0xEA,0xEA,0x60]),0x8000,16,"test")
        self.assertEqual(r[0]["mnemonic"],"LDA")
        self.assertEqual(r[0]["operand"],"#0x01")
        self.assertEqual(r[1]["target_cpu_address"],"0x8006")
        self.assertEqual(r[-1]["mnemonic"],"RTS")
    def test_unknown_opcode_preserves_alignment(self):
        r=disasm.disassemble(bytes([0x02,0xA9,0x10]),0x8000,0,"test")
        self.assertEqual(r[0]["mnemonic"],".byte")
        self.assertEqual(r[1]["file_offset"],1)
        self.assertEqual(r[1]["mnemonic"],"LDA")
    def test_nrom_bank_windows(self):
        banks=disasm.prg_banks(bytes(32768),0)
        self.assertEqual(len(banks),2)
        self.assertEqual(banks[0]["cpu_base"],0x8000)
        self.assertEqual(banks[1]["cpu_base"],0xC000)
class PipelineTests(unittest.TestCase):
    def test_chr_duplicate_patterns(self):
        r=workbench.graphics(bytes(16)+bytes(16)+bytes(range(16)))
        self.assertEqual(r["complete_tile_count"],3)
        self.assertEqual(r["unique_tile_patterns"],2)
        self.assertEqual(r["duplicate_pattern_groups"][0]["occurrences"],2)
    def test_oam_snapshot_sprite_extraction(self):
        oam=[0]*256
        oam[0:4]=[9,17,0xC2,44]
        result=workbench._oam_summary([{"frame":3,"oam":oam}])
        self.assertEqual(result["snapshot_count"],1)
        sprite=result["snapshots"][0]["sprites"][0]
        self.assertEqual(sprite["x"],44)
        self.assertEqual(sprite["y"],10)
        self.assertTrue(sprite["flip_horizontal"])
        self.assertTrue(sprite["flip_vertical"])
    def test_trace_json_aggregation(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/"trace.json"
            p.write_text('[{"pc":"0x8000","input":"A"},{"pc":"0x8000","address":"0x0002","value":"0x01"}]')
            r=workbench.trace_data(p)
            self.assertEqual(r["event_count"],2)
            self.assertEqual(r["hot_program_counters"][0]["hits"],2)
            self.assertEqual(r["frequent_memory_writes"][0]["address"],"0x0002")
if __name__=="__main__": unittest.main()
