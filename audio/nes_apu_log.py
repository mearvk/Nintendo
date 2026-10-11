#!/usr/bin/env python3
"""Validate CSV NES APU write logs (cycle,address,value); does not emulate sound."""
import argparse,csv,json
from collections import Counter
from pathlib import Path
REGS=set(range(0x4000,0x4014))|{0x4015,0x4017}
def parse(path):
 events=[]; prev=-1
 with path.open(newline="",encoding="utf-8") as f:
  r=csv.DictReader(f)
  if not r.fieldnames or not {"cycle","address","value"}.issubset(r.fieldnames):raise ValueError("CSV needs cycle,address,value columns")
  for line,row in enumerate(r,2):
   try:cycle=int(row["cycle"],0); addr=int(row["address"],0); value=int(row["value"],0)
   except (TypeError,ValueError) as e:raise ValueError("line %d: values must be integers"%line) from e
   if cycle<0 or cycle<prev:raise ValueError("line %d: cycle must be nonnegative and nondecreasing"%line)
   if addr not in REGS:raise ValueError("line %d: register address is not accepted"%line)
   if not 0<=value<=255:raise ValueError("line %d: value must be 0..255"%line)
   events.append({"cycle":cycle,"address":addr,"value":value}); prev=cycle
 return events
def main():
 p=argparse.ArgumentParser(description=__doc__); p.add_argument("log",type=Path); p.add_argument("--json",dest="out",type=Path); a=p.parse_args(); ev=parse(a.log)
 summary={"event_count":len(ev),"first_cycle":ev[0]["cycle"] if ev else None,"last_cycle":ev[-1]["cycle"] if ev else None,"writes_by_register":{"0x%04X"%k:v for k,v in sorted(Counter(x["address"] for x in ev).items())},"events":ev,"limitations":"Register-write validation only; no APU emulation or waveform reconstruction."}
 text=json.dumps(summary,indent=2)+"\n"
 if a.out:a.out.parent.mkdir(parents=True,exist_ok=True); a.out.write_text(text,encoding="utf-8")
 else:print(text,end="")
 return 0
if __name__=="__main__":
 try:raise SystemExit(main())
 except (OSError,ValueError) as e:print("error: "+str(e)); raise SystemExit(2)
