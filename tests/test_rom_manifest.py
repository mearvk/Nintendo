import json,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; TOOL=ROOT/"tools"/"rom_manifest.py"
class Tests(unittest.TestCase):
 def test_create_verify_and_detect_tampering(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d); image=p/"fixture.bin"; manifest=p/"fixture.json"; image.write_bytes(b"fixture")
   cmd=[sys.executable,str(TOOL),"create",str(image),str(manifest),"--title","Synthetic","--platform","test","--provenance","unit test"]
   subprocess.run(cmd,check=True,capture_output=True); self.assertFalse(json.loads(manifest.read_text())["redistribution_allowed"])
   subprocess.run([sys.executable,str(TOOL),"verify",str(manifest),str(image)],check=True,capture_output=True)
   image.write_bytes(b"tampered"); r=subprocess.run([sys.executable,str(TOOL),"verify",str(manifest),str(image)],capture_output=True,text=True)
   self.assertEqual(r.returncode,1); self.assertIn("sha256 mismatch",r.stdout)
 def test_rights_guard(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d); image=p/"x"; m=p/"x.json"; image.write_bytes(b"x")
   r=subprocess.run([sys.executable,str(TOOL),"create",str(image),str(m),"--title","x","--platform","x","--provenance","test","--redistribution-allowed"],capture_output=True,text=True)
   self.assertEqual(r.returncode,2); self.assertIn("conflicts",r.stdout)
if __name__=="__main__":unittest.main()
