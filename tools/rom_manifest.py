#!/usr/bin/env python3
"""Create/verify SHA-256 ROM metadata sidecars; source images are never modified."""
import argparse, hashlib, json
from datetime import datetime, timezone
from pathlib import Path
RIGHTS={"unknown","personal_dump","authorized","redistribution_approved","public_domain","original_homebrew"}
def sha(path):
 h=hashlib.sha256()
 with path.open("rb") as f:
  for b in iter(lambda:f.read(1048576),b""): h.update(b)
 return h.hexdigest()
def errors(m,image):
 e=[]; req=("schema_version","title","platform","format","size_bytes","sha256","provenance","rights_status","redistribution_allowed")
 e += ["missing required field: "+k for k in req if k not in m]
 if e:return e
 if m["schema_version"]!=1:e.append("schema_version must be 1")
 for k in ("title","platform","format","provenance"):
  if not isinstance(m[k],str) or not m[k].strip():e.append(k+" must be non-empty")
 if m["rights_status"] not in RIGHTS:e.append("invalid rights_status")
 if not isinstance(m["redistribution_allowed"],bool):e.append("redistribution_allowed must be boolean")
 elif m["redistribution_allowed"] and m["rights_status"] not in {"authorized","redistribution_approved","public_domain","original_homebrew"}:e.append("redistribution_allowed conflicts with rights_status")
 if not isinstance(m["size_bytes"],int) or isinstance(m["size_bytes"],bool):e.append("size_bytes must be integer")
 elif m["size_bytes"]!=image.stat().st_size:e.append("size mismatch")
 h=m["sha256"]
 if not isinstance(h,str) or len(h)!=64 or any(c not in "0123456789abcdefABCDEF" for c in h):e.append("sha256 must be 64 hexadecimal characters")
 elif h.lower()!=sha(image):e.append("sha256 mismatch")
 return e
def main():
 p=argparse.ArgumentParser(description=__doc__); s=p.add_subparsers(dest="cmd",required=True)
 c=s.add_parser("create"); c.add_argument("image",type=Path); c.add_argument("output",type=Path); c.add_argument("--title",required=True); c.add_argument("--platform",required=True); c.add_argument("--format",default="unknown"); c.add_argument("--region"); c.add_argument("--revision"); c.add_argument("--provenance",required=True); c.add_argument("--rights-status",choices=sorted(RIGHTS),default="unknown"); c.add_argument("--redistribution-allowed",action="store_true"); c.add_argument("--notes",default="")
 v=s.add_parser("verify"); v.add_argument("manifest",type=Path); v.add_argument("image",type=Path); a=p.parse_args()
 try:
  if a.cmd=="create":
   m={"schema_version":1,"title":a.title,"platform":a.platform,"region":a.region,"revision":a.revision,"format":a.format,"size_bytes":a.image.stat().st_size,"sha256":sha(a.image),"provenance":a.provenance,"rights_status":a.rights_status,"redistribution_allowed":a.redistribution_allowed,"analysis_tool":"tools/rom_manifest.py","tool_version":"1.0","analysis_timestamp":datetime.now(timezone.utc).isoformat(),"notes":a.notes}
   e=errors(m,a.image)
   if e: raise ValueError("; ".join(e))
   a.output.parent.mkdir(parents=True,exist_ok=True); a.output.write_text(json.dumps(m,indent=2)+"\n",encoding="utf-8"); print("manifest written; SHA-256 "+m["sha256"]); return 0
  m=json.loads(a.manifest.read_text(encoding="utf-8")); e=errors(m,a.image)
  if e: print("\n".join("FAIL: "+x for x in e)); return 1
  print("PASS: image matches manifest"); return 0
 except (OSError,ValueError,json.JSONDecodeError) as ex: print("error: "+str(ex)); return 2
if __name__=="__main__": raise SystemExit(main())
