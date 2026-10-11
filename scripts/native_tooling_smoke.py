import hashlib,json,subprocess,sys,tempfile,unittest
from pathlib import Path
BIN=Path(sys.argv[1] if len(sys.argv)>1 else "build")
class NativeTools(unittest.TestCase):
 def runx(self,*args,expect=0):
  r=subprocess.run([str(BIN/args[0]),*map(str,args[1:])],capture_output=True,text=True)
  self.assertEqual(r.returncode,expect,r.stdout+r.stderr);return r
 def test_manifest_hash_and_tamper(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td); image=p/"f.nes"; raw=b"NES\x1a"+bytes([1,1])+bytes(10)+bytes(16384+8192);image.write_bytes(raw)
   for exe in ("rom-manifest-c","rom-manifest-cpp"):
    m=p/(exe+".json");self.runx(exe,"create",image,m,"--title","fixture","--platform","NES","--provenance","synthetic")
    self.assertEqual(json.loads(m.read_text())["sha256"],hashlib.sha256(raw).hexdigest());self.runx(exe,"verify",m,image)
    image.write_bytes(raw+b"x");self.runx(exe,"verify",m,image,expect=1);image.write_bytes(raw)
 def test_chr(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td);raw=bytearray(b"NES\x1a"+bytes([1,1])+bytes(10)+bytes(16384+8192));raw[16+16384]=128;im=p/"f.nes";im.write_bytes(raw)
   for exe in ("nes-chr-c","nes-chr-cpp"):
    out=p/(exe+".ppm");self.runx(exe,im,out,"--tiles","1");self.assertTrue(out.read_bytes().startswith(b"P6\n8 8\n255\n"))
 def test_apu(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td);log=p/"a.csv";log.write_text("cycle,address,value\n0,0x4000,48\n12,0x4015,3\n")
   for exe in ("nes-apu-c","nes-apu-cpp"):
    out=p/(exe+".json");self.runx(exe,log,"--json",out);self.assertEqual(json.loads(out.read_text())["event_count"],2)
if __name__=="__main__":unittest.main(argv=[sys.argv[0]],verbosity=2)
