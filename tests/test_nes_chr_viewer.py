import importlib.util,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; SPEC=importlib.util.spec_from_file_location("chr",ROOT/"graphics"/"nes_chr_viewer.py"); M=importlib.util.module_from_spec(SPEC); SPEC.loader.exec_module(M)
class Tests(unittest.TestCase):
 def test_bitplanes(self):
  t=bytearray(16); t[0]=128; t[8]=64; w,h,p=M.decode(bytes(t),1); self.assertEqual((w,h),(8,8)); self.assertEqual(p[:3],bytes((85,)*3)); self.assertEqual(p[3:6],bytes((170,)*3))
 def test_bad_data(self):
  with self.assertRaises(ValueError):M.decode(b"x"*15)
  with self.assertRaises(ValueError):M.decode(b"x"*16,2)
  with self.assertRaises(ValueError):M.locate(b"bad")
if __name__=="__main__":unittest.main()
