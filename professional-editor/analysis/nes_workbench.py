#!/usr/bin/env python3
"""Local-first NES ROM analysis pipeline."""
from __future__ import annotations
import argparse, csv, html, importlib.util, json, re, sys
from collections import Counter
from pathlib import Path
import nes_circuit
HERE = Path(__file__).resolve().parent

def load(name, filename):
    spec = importlib.util.spec_from_file_location(name, HERE / filename)
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod
inspect_mod = load("nes_inspect", "nes_inspect.py")
disasm_mod = load("nes_disasm", "nes_disasm.py")

def _byte_array(value):
    if isinstance(value, list):
        try: return bytes(int(x) & 255 for x in value)
        except (ValueError, TypeError): return None
    if isinstance(value, str):
        text=value.strip().replace("0x","").replace(" ","").replace(",","")
        try: return bytes.fromhex(text)
        except ValueError: return None
    return None

def _oam_summary(events):
    snapshots=[]
    for idx,event in enumerate(events):
        if not isinstance(event,dict) or "oam" not in event: continue
        raw=_byte_array(event["oam"])
        if raw is None or len(raw)<256: continue
        sprites=[]
        for n in range(64):
            y,tile,attr,x=raw[n*4:n*4+4]
            sprites.append({"index":n,"x":x,"y":y+1,"tile":tile,"attributes":attr,
                "palette":attr&3,"behind_background":bool(attr&0x20),
                "flip_horizontal":bool(attr&0x40),"flip_vertical":bool(attr&0x80)})
        snapshots.append({"event_index":idx,"frame":event.get("frame"),"sprites":sprites})
    changes=[]
    for previous,current in zip(snapshots,snapshots[1:]):
        changed=[s["index"] for s,p in zip(current["sprites"],previous["sprites"]) if s!=p]
        changes.append({"from_event":previous["event_index"],"to_event":current["event_index"],
                        "changed_sprite_indices":changed,"changed_count":len(changed)})
    return {"snapshot_count":len(snapshots),"snapshots":snapshots[:100],
            "between_snapshot_changes":changes[:100],
            "note":"Requires an emulator trace event with an OAM field containing 256 byte values or a 512-character hex string."}

def _ppu_snapshot_summary(events):
    snapshots=[]
    for idx,event in enumerate(events):
        if not isinstance(event,dict): continue
        item={"event_index":idx,"frame":event.get("frame")}
        palette=_byte_array(event.get("palette")) if "palette" in event else None
        nametable=_byte_array(event.get("nametable")) if "nametable" in event else None
        if palette is not None and len(palette)>=32:
            item["palette_bytes"]=list(palette[:32])
            item["palette_note"]="Raw palette bytes; emphasis and mirroring interpretation depends on emulator export."
        if nametable is not None and len(nametable)>=960:
            item["nametable_tile_grid"]=[list(nametable[y*32:(y+1)*32]) for y in range(30)]
            item["nametable_note"]="First 960 bytes shown as a 32x30 tile-index grid; attribute bytes are not included in this grid."
        if len(item)>2: snapshots.append(item)
    return {"snapshot_count":len(snapshots),"snapshots":snapshots[:100],
            "note":"Requires emulator trace fields named palette and/or nametable containing byte arrays or hex strings."}

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
        "events":events[:10000], "oam_analysis":_oam_summary(events), "ppu_snapshot_analysis":_ppu_snapshot_summary(events),
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

def reachable_nrom(prg, file_base):
    """Conservative recursive traversal for NROM only; banked mappers need stateful mapping."""
    if len(prg) not in (16384,32768) or len(prg)<6:
        return {"supported":False,"instructions":[],"note":"Reachability traversal is currently limited to standard 16/32 KiB NROM."}
    def map_address(address):
        if not 0x8000 <= address <= 0xFFFF: return None
        if len(prg)==16384: return (address-0x8000) % 16384
        return address-0x8000
    vector_offset=len(prg)-6
    seeds=[]
    for name,pos in (("nmi",vector_offset),("reset",vector_offset+2),("irq_brk",vector_offset+4)):
        address=prg[pos] | (prg[pos+1]<<8)
        if map_address(address) is not None: seeds.append({"vector":name,"address":address})
    queue=[x["address"] for x in seeds]
    visited=set(); result=[]
    while queue and len(result)<100000:
        address=queue.pop(0)
        off=map_address(address)
        if off is None or off in visited: continue
        visited.add(off)
        ins_list=disasm_mod.disassemble(prg[off:min(len(prg),off+3)],address,file_base+off,"NROM-reachable",limit=3)
        if not ins_list: continue
        ins=ins_list[0]
        # If a truncated operand reaches the end of the selected PRG image, stop this path.
        if ins.get("classification")=="truncated_instruction": continue
        ins["reachability"]="reachable_candidate"
        ins["seed_vectors"]=seeds
        result.append(ins)
        next_addr=(address+ins["size"])&0xFFFF
        target=ins.get("target_cpu_address")
        target_addr=int(target,16) if target else None
        flow=ins.get("flow")
        if flow=="conditional_branch":
            if target_addr is not None: queue.append(target_addr)
            queue.append(next_addr)
        elif flow=="call":
            if target_addr is not None: queue.append(target_addr)
            queue.append(next_addr)
        elif flow=="jump":
            if target_addr is not None and ins.get("mnemonic")=="JMP" and not ins.get("operand","").startswith("("):
                queue.append(target_addr)
        elif flow not in ("return",) and ins.get("mnemonic")!="BRK":
            queue.append(next_addr)
    return {"supported":True,"seed_vectors":seeds,"visited_byte_offsets":len(visited),
        "instructions":sorted(result,key=lambda x:x["file_offset"]),
        "note":"Reachability is a candidate for NROM only. Indirect JMP targets and data/code ambiguity remain unresolved."}

def hardware_register_candidates(instructions):
    names={0x2000:"PPUCTRL",0x2001:"PPUMASK",0x2002:"PPUSTATUS",0x2003:"OAMADDR",
           0x2004:"OAMDATA",0x2005:"PPUSCROLL",0x2006:"PPUADDR",0x2007:"PPUDATA",
           0x4014:"OAMDMA",0x4015:"APUSTATUS",0x4016:"CONTROLLER1_OR_STROBE",
           0x4017:"CONTROLLER2_OR_FRAME_COUNTER"}
    out=[]
    for ins in instructions:
        operand=ins.get("operand","")
        match=re.match(r"^0x([0-9A-Fa-f]{4})(?:,|$)",operand)
        if match:
            addr=int(match.group(1),16)
            if addr in names:
                out.append({"register":names[addr],"address":f"0x{addr:04X}",
                    "mnemonic":ins.get("mnemonic"),"file_offset":ins.get("file_offset"),
                    "cpu_address_candidate":ins.get("cpu_address_candidate"),
                    "confidence":"medium","note":"Address matches a standard NES I/O register; confirm instruction context and mapper/hardware variant."})
    return out

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
    reachable=reachable_nrom(prg,prgoff) if mapper==0 else {"supported":False,"instructions":[],"note":"Recursive traversal not yet enabled for this mapper."}
    hardware=hardware_register_candidates(instructions)
    circuit=nes_circuit.analyze_circuit(instructions, trace)
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
        "graphics":graphics(chrdata),"disassembly":{"bank_count":len(bankresults),"instructions":instructions,"reachable_nrom":reachable},
        "hardware_register_candidates":hardware,
        "control_flow_candidates":{"calls":calls[:5000],"jumps":jumps[:5000],"branches":branches[:10000],
            "stores":stores[:10000],"immediate_constants":immediates[:10000],
            "note":"Linear sweep may decode data as instructions; no function boundaries are proven."},
        "gameplay_hypotheses":hypotheses[:15000],"circuit_analysis":circuit,"trace_analysis":trace,
        "confidence_policy":{"high":"directly verified byte or format fact","medium":"structural clue or trace observation","low":"heuristic needing review"},
        "limitations":["Linear disassembly can decode data as instructions.","Bank switching is approximated; unsupported mapper layouts need profiles.",
            "Sprite/background reconstruction requires palettes, nametable/attribute data, OAM and runtime PPU observations.",
            "Settings and RAM address meanings are not proven without evidence and dynamic validation.",
            "This is static analysis, not a full emulator or source decompiler."]}
    (out/"analysis.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    (out/"circuit-analysis.json").write_text(json.dumps(circuit,indent=2)+"\n",encoding="utf-8")
    (out/"circuit.dot").write_text(nes_circuit.to_dot(circuit)+"\n",encoding="utf-8")
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
    image_block = '<img class="tiles" src="chr-tiles.png" alt="Grayscale raw CHR tile sheet">' if (out/"chr-tiles.png").exists() else '<p>No CHR-ROM tile sheet: the ROM may use CHR-RAM or have no available CHR-ROM bytes.</p>'
    oam_rows = ""
    if trace and trace.get("oam_analysis",{}).get("snapshots"):
        first = trace["oam_analysis"]["snapshots"][0]
        oam_rows = "<h2>First OAM snapshot</h2><p>Sprite records show hardware OAM positions and attributes, not a fully rendered sprite.</p><table><thead><tr><th>ID</th><th>X</th><th>Y</th><th>Tile</th><th>Palette</th><th>Flags</th></tr></thead><tbody>"
        for sprite in first["sprites"]:
            flags = ", ".join(label for label,key in (("H-flip","flip_horizontal"),("V-flip","flip_vertical"),("behind BG","behind_background")) if sprite[key]) or "—"
            oam_rows += "<tr>" + "".join("<td>"+html.escape(str(v))+"</td>" for v in (sprite["index"],sprite["x"],sprite["y"],f"0x{sprite['tile']:02X}",sprite["palette"],flags)) + "</tr>"
        oam_rows += "</tbody></table>"
    html_page = """<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>NES ROM analysis</title>
<style>body{font:16px system-ui,sans-serif;max-width:1100px;margin:2rem auto;padding:0 1rem;line-height:1.5;color:#202124}h1,h2{line-height:1.2}.tiles{max-width:100%;image-rendering:pixelated;border:1px solid #888;background:#ddd}nav a{margin-right:1rem}table{border-collapse:collapse}th,td{border:1px solid #aaa;padding:.35rem .55rem;text-align:left}tbody tr:nth-child(even){background:#f4f4f4}.note{padding:.8rem;background:#f5f5f5}</style></head><body>
<h1>NES ROM analysis: """ + html.escape(rom.name) + """</h1>
<nav><a href="analysis.json">Machine report (JSON)</a><a href="circuit-analysis.json">Circuit graph (JSON)</a><a href="circuit.dot">Graphviz DOT</a><a href="disassembly.txt">Candidate disassembly</a><a href="analysis-summary.md">Summary (Markdown)</a></nav>
<p class="note">Static analysis contains hypotheses, not verified source code. Bank mapping and code/data boundaries can be uncertain.</p>
<h2>Container and code overview</h2><ul><li>Mapper: """ + str(mapper) + """</li><li>PRG bank windows: """ + str(len(bankresults)) + """</li><li>Instruction candidates: """ + str(len(instructions)) + """</li><li>Unique CHR patterns: """ + str(result["graphics"]["unique_tile_patterns"]) + """</li><li>Gameplay/state hypotheses: """ + str(len(result["gameplay_hypotheses"])) + """</li></ul>
<h2>Circuit-style control-flow analysis</h2><p>Candidate nodes: """ + str(circuit["node_count"]) + """; edges: """ + str(circuit["edge_count"]) + """; loop/repetition candidates: """ + str(len(circuit["loops"]) + len(circuit["repeated_instruction_runs"])) + """; action assessments: """ + str(len(circuit["action_assessments"])) + """. See circuit-analysis.json for sequence edges, branch paths, repeat candidates, and evidence-led action assessments. A return path is not proof of a win state or morally correct action.</p><h2>Raw CHR-ROM tile sheet</h2>""" + image_block + oam_rows + """<h2>Analysis caveats</h2><ul>""" + "".join("<li>"+html.escape(x)+"</li>" for x in result["limitations"]) + """</ul></body></html>"""
    (out/"index.html").write_text(html_page,encoding="utf-8")
    return {"output":str(out),"mapper":mapper,"bank_count":len(bankresults),"instruction_candidates":len(instructions),
        "tile_count":result["graphics"]["complete_tile_count"],"circuit_nodes":circuit["node_count"],"circuit_edges":circuit["edge_count"],"loop_candidates":len(circuit["loops"])}

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
