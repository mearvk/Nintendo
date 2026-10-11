import importlib.util,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; SPEC=importlib.util.spec_from_file_location("apu",ROOT/"audio"/"nes_apu_log.py"); M=importlib.util.module_from_spec(SPEC); SPEC.loader.exec_module(M)
class Tests(unittest.TestCase):
 def log(self,body):
  d=tempfile.TemporaryDirectory(); self.addCleanup(d.cleanup); p=Path(d.name)/"w.csv"; p.write_text("cycle,address,value\n"+body); return p
 def test_valid(self):self.assertEqual(len(M.parse(self.log("0,0x4000,0x30\n12,0x4015,3\n"))),2)
 def test_invalid(self):
  with self.assertRaisesRegex(ValueError,"nondecreasing"):M.parse(self.log("10,0x4000,1\n9,0x4001,2\n"))
  with self.assertRaisesRegex(ValueError,"not accepted"):M.parse(self.log("0,0x4014,1\n"))
  with self.assertRaisesRegex(ValueError,"0..255"):M.parse(self.log("0,0x4015,256\n"))
if __name__=="__main__":unittest.main()
