#!/usr/bin/env python3
"""Local-first NES ROM analysis pipeline."""
from __future__ import annotations
import argparse, csv, importlib.util, json, re, sys
from collections import Counter
from pathlib import Path
HERE = Path(__file__).resolve().parent

def load(name, filename):
    spec = importlib.util.spec_from_file_location(name, HERE / filename)
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod
inspect_mod = load("nes_inspect", "nes_inspect.py")
disasm_mod = load("nes_disasm", "nes_disasm.py")

def trace_data(path):
    raw = path.read_text(encoding="utf-8")
    rows = list(csv.DictReader(raw.splitlines())) if path.suffix.lower()==".csv" else json.loads(raw)
    if isinstance(rows,dict): rows=rows.get("events",[])
    pcs, writes, inputs = Counter(), Counter(), Counter()
    events=[]
    for row in rows:
        if not isinstance(row,dict): continue
        item=dict(row)
        for key in ("pc","address","value"):
            if key in item:
                try: item[key]=int(str(item[key]),0)
                except (ValueError,TypeError): pass
        if isinstance(item.get("pc"),int): pcs[item["pc"]]+=1
        if "input" in item: inputs[str(item["input"])]+=1
        if isinstance(item.get("address"),int) and isinstance(item.get("value"),int):
            writes[(item["address"],item["value"])]+=1
        events.append(item)
    return {"source":str(path),"event_count":len(events),
        "hot_program_counters":[{"pc":f"0x{k:04X}","hits":v} for k,v in pcs.most_common(100)],
        "input_values":dict(inputs),
        "frequent_memory_writes":[{"address":f"0x{a:04X}","value":f"0x{v:02X}","count":n} for (a,v),n in writes.most_common(100)],
        "events":events[:10000],
        "note":"Aggregates supplied emulator exports; this tool does not launch an emulator."}

def graphics(chr_data):
    usable=len(chr_data)//16*16
    tiles=[chr_data[i:i+16] for i in range(0,usable,16)]
    counts=Counter(tiles); first={}; groups=[]
    for i,t in enumerate(tiles):
        if t not in first: first[t]=i
    for t,n in counts.items():
        if n>1: groups.append({"first_tile_index":first[t],"occurrences":n,"confidence":"high","evidence":"identical 16-byte CHR pattern"})
    return {"complete_tile_count":len(tiles),"unique_tile_patterns":len(counts),
        "incomplete_trailing_bytes":len(chr_data)-usable,"duplicate_pattern_groups":groups[:1000],
        "note":"Raw CHR patterns do not establish palette, sprite grouping, or screen usage."}

def analyze(rom, outroot, tracepath=None):
    raw=rom.read_bytes()
    base=inspect_mod.inspect(rom,outroot)
    h=raw[:16]; trainer=bool(h[6]&4)
    prgoff=16+(512 if trainer else 0)
    prgsize=base["header"]["prg_rom_bytes_declared"]; chrsize=base["header"]["chr_rom_bytes_declared"]
    prg=raw[prgoff:min(len(raw),prgoff+prgsize)]
    chroff=prgoff+prgsize
    chrdata=raw[chroff:min(len(raw),chroff+chrsize)] if chrsize else b""
    mapper=base["header"]["mapper"]; bankresults=[]; instructions=[]
    for bank in disasm_mod.prg_banks(prg,mapper):
        ins=disasm_mod.disassemble(bank["data"],bank["cpu_base"],prgoff+bank["file_offset"],bank["name"])
        bankresults.append({"name":bank["name"],"file_offset":prgoff+bank["file_offset"],"size":len(bank["data"]),
            "cpu_base_candidate":f"0x{bank['cpu_base']:04X}","instructions":ins})
        instructions.extend(ins)
    trace=trace_data(tracepath) if tracepath else None
    calls=[i for i in instructions if i.get("mnemonic")=="JSR"]
    jumps=[i for i in instructions if i.get("mnemonic")=="JMP"]
    branches=[i for i in instructions if i.get("flow")=="conditional_branch"]
    stores=[i for i in instructions if i.get("mnemonic") in ("STA","STX","STY")]
    immediates=[i for i in instructions if i.get("mnemonic") in ("LDA","LDX","LDY","CMP","CPX","CPY","ADC","SBC") and i.get("operand","").startswith("#")]
    hypotheses=[]
    for i in instructions:
        m=i.get("mnemonic","")
        if m in ("STA","STX","STY"):
            category="state or hardware-register write candidate"
            note="Store destination needs RAM/PPU/APU/mapper address mapping and execution evidence."
        elif m in ("CMP","CPX","CPY"):
            category="threshold or rule candidate"
            note="Comparison may be a difficulty rule, collision check, or ordinary loop bound."
        elif m in ("ADC","SBC","INC","DEC","ASL","LSR","ROL","ROR"):
            category="movement, timer, score, or arithmetic candidate"
            note="Arithmetic purpose cannot be inferred from opcode alone."
        elif m in ("LDA","LDX","LDY","BIT"):
            category="input or state read candidate"
            note="Read may involve input or state; confirm against mapped addresses and traces."
        else: continue
        hypotheses.append({"category":category,"file_offset":i["file_offset"],"cpu_address_candidate":i["cpu_address_candidate"],
            "mnemonic":m,"operand":i.get("operand",""),"bytes":i.get("bytes",[]),"confidence":"low","note":note})
    if trace:
        for x in trace["hot_program_counters"][:50]:
            hypotheses.append({"category":"runtime-hot-code candidate","cpu_address_candidate":x["pc"],"hits":x["hits"],
                "confidence":"medium","evidence":"supplied emulator trace","note":"Frequently executed PC; semantic role is unconfirmed."})
        for x in trace["frequent_memory_writes"][:50]:
            hypotheses.append({"category":"runtime memory-write candidate","address":x["address"],"value":x["value"],
                "count":x["count"],"confidence":"low","evidence":"supplied emulator trace",
                "note":"May be RAM, PPU, APU, or mapper I/O; memory map context is required."})
    out=outroot/(rom.stem or "rom"); out.mkdir(parents=True,exist_ok=True)
    result={"schema_version":"1.0","input":str(rom.resolve()),"container":base,
        "mapper_analysis":{"mapper":mapper,"bank_windows":[{k:v for k,v in b.items() if k!="data"} for b in disasm_mod.prg_banks(prg,mapper)],
            "note":"Bank windows are approximate views; mapper register state is not emulated."},
        "graphics":graphics(chrdata),"disassembly":{"bank_count":len(bankresults),"instructions":instructions},
        "control_flow_candidates":{"calls":calls[:5000],"jumps":jumps[:5000],"branches":branches[:10000],
            "stores":stores[:10000],"immediate_constants":immediates[:10000],
            "note":"Linear sweep may decode data as instructions; no function boundaries are proven."},
        "gameplay_hypotheses":hypotheses[:15000],"trace_analysis":trace,
        "confidence_policy":{"high":"directly verified byte or format fact","medium":"structural clue or trace observation","low":"heuristic needing review"},
        "limitations":["Linear disassembly can decode data as instructions.","Bank switching is approximated; unsupported mapper layouts need profiles.",
            "Sprite/background reconstruction requires palettes, nametable/attribute data, OAM and runtime PPU observations.",
            "Settings and RAM address meanings are not proven without evidence and dynamic validation.",
            "This is static analysis, not a full emulator or source decompiler."]}
    (out/"analysis.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    with (out/"disassembly.txt").open("w",encoding="utf-8") as f:
        for bank in bankresults:
            f.write(f"\n; {bank['name']} file offset 0x{bank['file_offset']:X} size {bank['size']}\n")
            for i in bank["instructions"]:
                bs=" ".join(f"{b:02X}" for b in i["bytes"])
                f.write(f"{i['cpu_address_candidate']} [{bs:<8}] {i['mnemonic']:<5} {i.get('operand','')} ; file+0x{i['file_offset']:X}\n")
    summary=["# NES ROM analysis summary","",f"- Input: {rom.name}",f"- Mapper: {mapper}",
        f"- PRG bank windows: {len(bankresults)}",f"- Instruction candidates: {len(instructions)}",
        f"- Complete CHR tiles: {result['graphics']['complete_tile_count']}",
        f"- Unique CHR patterns: {result['graphics']['unique_tile_patterns']}",
        f"- Gameplay hypotheses: {len(result['gameplay_hypotheses'])}","","## Limitations"]
    summary += ["- "+x for x in result["limitations"]]
    summary += ["","See analysis.json for evidence and confidence. disassembly.txt is a candidate linear sweep, not verified source reconstruction."]
    (out/"analysis-summary.md").write_text("\n".join(summary)+"\n",encoding="utf-8")
    return {"output":str(out),"mapper":mapper,"bank_count":len(bankresults),"instruction_candidates":len(instructions),
        "tile_count":result["graphics"]["complete_tile_count"]}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("roms",nargs="+",type=Path)
    p.add_argument("--out-dir",type=Path,default=Path("rom-analysis-output"))
    p.add_argument("--trace",type=Path,help="optional emulator-exported JSON or CSV trace")
    a=p.parse_args(); failed=0
    for rom in a.roms:
        try:
            r=analyze(rom,a.out_dir,a.trace)
            print(f"OK {rom} -> {r['output']} | mapper {r['mapper']} | banks {r['bank_count']} | instructions {r['instruction_candidates']}")
        except (OSError,ValueError,json.JSONDecodeError) as e:
            failed+=1; print(f"ERR {rom}: {e}",file=sys.stderr)
    return 1 if failed else 0
if __name__=="__main__": raise SystemExit(main())
