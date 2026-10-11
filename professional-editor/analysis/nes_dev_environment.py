#!/usr/bin/env python3
"""Generate a per-title NES ROM development workspace."""
import argparse, csv, json, re, shutil, sys
from datetime import datetime, timezone
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import nes_workbench
import nes_advanced
import nes_rom_build

def slugify(value):
    return re.sub(r"[^A-Za-z0-9._-]+", "-", value.strip()).strip(".-_")[:96] or "Untitled"

def write_text(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content.rstrip() + "\n", encoding="utf-8")

def generate(rom, output_root, title=None, trace=None, force=False, rules_path=None):
    rom = Path(rom).expanduser().resolve()
    if not rom.is_file():
        raise FileNotFoundError(f"ROM does not exist or is not a file: {rom}")
    title = (title or rom.stem or "Untitled").strip() or "Untitled"
    provided_rules = nes_advanced.load_rules(rules_path) if rules_path else None
    root = Path(output_root).expanduser().resolve()
    root.mkdir(parents=True, exist_ok=True)
    target = root / ("development-environment-" + slugify(title))
    if target.exists() and not force:
        raise FileExistsError(f"Output exists: {target}; use --force to replace it")
    if target.exists():
        if target.is_symlink() or target.resolve().parent != root:
            raise ValueError("Refusing to replace a path outside the output root")
        shutil.rmtree(target)
    stage = root / (".nes-analysis-stage-" + slugify(title))
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir()
    try:
        summary = nes_workbench.analyze(rom, stage, trace)
        src = Path(summary["output"])
        analysis = json.loads((src / "analysis.json").read_text(encoding="utf-8"))
        container = analysis.get("container", {})
        header = container.get("header", {})
        instructions = analysis.get("disassembly", {}).get("instructions", [])
        rules = provided_rules if provided_rules is not None else nes_advanced.rules_template(title)
        rules.setdefault("title", title)
        advanced = nes_advanced.analyze_advanced(
            header.get("mapper"), int(header.get("prg_rom_bytes_declared") or 0),
            instructions, analysis.get("trace_analysis"), rules,
            int(header.get("chr_rom_bytes_declared") or 0))
        analysis["advanced_analysis"] = advanced
        write_text(src / "analysis.json", json.dumps(analysis, indent=2))
        write_text(src / "advanced-analysis.json", json.dumps(advanced, indent=2))
        for folder in ("source-rom", "sprites", "game-logic", "codes/disassembly", "codes/circuit", "traces", "reports"):
            (target / folder).mkdir(parents=True, exist_ok=True)
        shutil.copy2(src / "report.json", target / "source-rom/rom-metadata.json")
        for source, dest in (("analysis.json","reports/analysis.json"), ("analysis-summary.md","reports/analysis-summary.md"),
                             ("index.html","reports/analysis.html"), ("disassembly.txt","codes/disassembly/linear-sweep.txt"),
                             ("analysis.json","codes/mapper-analysis.json"), ("circuit-analysis.json","codes/circuit/circuit-analysis.json"), ("circuit.dot","codes/circuit/circuit.dot"), ("advanced-analysis.json","codes/advanced-analysis.json")):
            shutil.copy2(src / source, target / dest)
        for name in ("chr-tiles.png", "chr-tiles.pgm"):
            if (src / name).is_file():
                shutil.copy2(src / name, target / ("sprites/background-tiles" + (".png" if name.endswith(".png") else ".pgm")))
        if (target / "sprites/background-tiles.png").exists():
            shutil.copy2(target / "sprites/background-tiles.png", target / "sprites/sprite-map.png")
        raw = rom.read_bytes()
        chr_size = int(header.get("chr_rom_bytes_declared", 0))
        chr_start = 16 + (512 if raw[6] & 4 else 0) + int(header.get("prg_rom_bytes_declared", 0))
        chr_data = raw[chr_start:chr_start + chr_size] if chr_size else b""
        seen = {}
        with (target / "sprites/sprite-catalog.csv").open("w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["tile_index","chr_offset_hex","pattern_hex","pattern_id","note"])
            for i in range(len(chr_data)//16):
                pattern = chr_data[i*16:(i+1)*16].hex()
                if pattern not in seen: seen[pattern] = len(seen)
                writer.writerow([i, f"0x{i*16:04X}", pattern, seen[pattern], "raw CHR tile; palette/sprite grouping unknown"])
        hypotheses = analysis.get("gameplay_hypotheses", [])
        lines = ["# Game Logic and Gameplay Hypotheses", "", "Static-analysis leads only, not recovered original source.", "",
                 f"- Mapper: {header.get('mapper','unknown')}", f"- Instruction candidates: {summary.get('instruction_candidates',0)}",
                 f"- Hypotheses: {len(hypotheses)}", "", "| Category | Location | Instruction | Confidence | Note |", "|---|---|---|---|---|"]
        for x in hypotheses[:5000]:
            loc = x.get("cpu_address_candidate", x.get("address", x.get("file_offset","unknown")))
            ins = " ".join(str(x.get(k,"")) for k in ("mnemonic","operand")).strip() or "—"
            note = str(x.get("note","")).replace("|","\\|").replace("\n"," ")
            lines.append(f"| {x.get('category','candidate')} | {loc} | {ins} | {x.get('confidence','unknown')} | {note} |")
        lines += ["", "## Validation", "", "1. Compare writes with the NES memory map and mapper.",
                  "2. Capture emulator traces for boot, menus, gameplay, pause, and transitions.",
                  "3. Correlate input changes with hot PCs and memory writes.",
                  "4. Document confirmed variables with offsets, trace evidence, and confidence.",
                  "5. Keep unverified interpretations labeled as hypotheses."]
        write_text(target / "game-logic/gameplay-hypotheses.md", "\n".join(lines))
        write_text(target / "game-logic/overview.md", f"# Game Logic Overview\n\nTitle: {title}\nMapper: {header.get('mapper')}\nPRG bytes: {header.get('prg_rom_bytes_declared')}\nCHR bytes: {header.get('chr_rom_bytes_declared')}\nInstruction candidates: {summary.get('instruction_candidates')}\nHypotheses: {len(hypotheses)}\n\nStart with gameplay-hypotheses.md and validate with runtime traces.")
        circuit = analysis.get("circuit_analysis", {})
        action_lines = ["# Action Analysis: Preconditions, Order, Repetition, and Outcomes", "",
            "## How to interpret this report", "",
            "This report distinguishes observable code structure from inferred intent. A branch is a decision candidate; a backward edge is a loop candidate; an instruction sequence is only a candidate order. None alone proves a game objective, an ideal path, or moral correctness.", "",
            "For each candidate action, establish: (1) preconditions and branch guards; (2) the intended objective; (3) state changes and side effects; (4) constraints/invariants; (5) whether repetition is intended, bounded, or accidental; (6) failure/recovery paths; and (7) runtime evidence from controlled traces.", "",
            "## Action candidates", "",
            "| Address | Instruction | Category | Repeat candidate | Terminal path | Runtime evidence | Right-action status |", "|---|---|---|---|---|---|---|"]
        for action in circuit.get("action_assessments", [])[:5000]:
            checks = {x.get("criterion"): x.get("status") for x in action.get("checks", [])}
            vals = [action.get("address","?"), action.get("instruction",""), action.get("category",""),
                    str(action.get("in_repeat_candidate",False)).lower(),
                    checks.get("can reach return/interrupt-return/BRK candidate","not-established"),
                    checks.get("observed at runtime","no-matching-trace-evidence"),
                    action.get("right_action_assessment",{}).get("status","undetermined")]
            action_lines.append("| " + " | ".join(str(v).replace("|","\\\\|").replace("\\n"," ") for v in vals) + " |")
        action_lines += ["", "## What 'right action' means here", "",
            "The analyzer does not assign an intrinsic moral verdict to opcodes. It records evidence that can support a testable engineering judgment: an action is provisionally right for a specified game rule when its preconditions are satisfied, its effects advance the documented objective, it respects invariants and constraints, and runtime tests confirm the expected result. If the objective or constraints are unknown, the judgment remains undetermined.", "",
            "A path toward RTS/RTI/BRK is not a win-state detector. Identifying win, loss, scoring, collision, menu, or progression logic requires semantic mapping and runtime evidence.", "",
            "See ../codes/circuit/circuit-analysis.json for the node/edge graph and ../codes/circuit/circuit.dot for Graphviz rendering."]
        write_text(target / "game-logic/action-assessment.md", "\n".join(action_lines))
        write_text(target / "game-logic/state-and-events.md", "# State and Events Notebook\n\n| State/event | Evidence | Trigger | Effect | Confidence |\n|---|---|---|---|---|\n| TODO | TODO | TODO | TODO | unverified |")
        write_text(target / "game-logic/memory-map.md", "# Memory Map Investigation\n\nRecord address, width, read/write sites, observed values, scene, evidence, and confidence. Game-specific RAM labels require validation.\n\n## Automated address classification\n\nSee ../codes/advanced-analysis.json for candidate RAM, PPU, APU, controller, expansion, save-RAM, and mapper-controlled address regions. Classification is address-based; cartridge-specific semantics still require evidence.")
        write_text(target / "game-logic/game-rules.json", json.dumps(rules, indent=2))
        write_text(target / "game-logic/rules-evaluation.json", json.dumps(advanced.get("rules_evaluation", {}), indent=2))
        write_text(target / "game-logic/rules-evaluation.md", "# Declared Game Rules — Evidence Check\n\nEdit game-rules.json to record the verified objective, constraints, action addresses, required/allowed/forbidden labels, and ordering expectations. Then regenerate the workspace with --rules path/to/game-rules.json to evaluate that ruleset.\n\nStatuses are consistent-with-declared-rule, conflicts-with-declared-rule, or unresolved. They report consistency with the declared rule and supplied evidence—not proof of complete game behavior.\n\n" + "\n".join(f"- **{x.get('address') or 'unknown'} {x.get('label','')}** — {x.get('status','unresolved')}: {'; '.join(x.get('evidence', []))}" for x in advanced.get("rules_evaluation", {}).get("checks", [])))
        regs = analysis.get("hardware_register_candidates", [])
        write_text(target / "codes/hardware-registers.md", "# NES Hardware Register Candidates\n\n" + ("\n".join(f"- {x.get('address')} {x.get('register')}: {x.get('mnemonic')} at file offset {x.get('file_offset')} ({x.get('confidence')})" for x in regs) or "No candidates emitted.") + "\n\nConfirm instruction context and mapper.")
        if trace:
            trace = Path(trace).expanduser().resolve()
            shutil.copy2(trace, target / "traces" / trace.name)
            if analysis.get("trace_analysis"):
                write_text(target / "traces/trace-summary.json", json.dumps(analysis["trace_analysis"], indent=2))
        else:
            write_text(target / "traces/README.md", "# Emulator Traces\n\nNo trace supplied. Rerun with --trace path/to/trace.json or CSV to correlate runtime behavior.")
        write_text(target / "title-description.md", f"# {title}\n\n## Title Description\n\nAdd a verified synopsis, genre, setting, objectives, stages, and mechanics. Static analysis cannot reliably infer the complete narrative.\n\n## Container Facts\n\nInput: {rom.name}\nFormat: {container.get('format')}\nMapper: {header.get('mapper')}\nPRG-ROM: {header.get('prg_rom_bytes_declared')} bytes\nCHR-ROM: {header.get('chr_rom_bytes_declared')} bytes\nMirroring: {header.get('mirroring')}\nTrainer: {header.get('trainer_present')}\n\n## Research Notes\n\n- Release region: TODO\n- Developer/publisher: TODO\n- Controls: TODO\n- Game modes and settings: TODO\n- Sources and verification: TODO")
        build_project = target / "rom-project"
        nes_rom_build.init_project(rom, build_project, title=title)
        manifest = {"schema_version":"1.0","title":title,"generated_utc":datetime.now(timezone.utc).isoformat(),
                    "input_filename":rom.name,"input_path":str(rom),"input_rom_copied":True,"baseline_copy_path":"rom-project/source/original.nes","round_trip_project":"rom-project/rom-project.json","format":container.get("format"),
                    "mapper":header.get("mapper"),"prg_rom_bytes_declared":header.get("prg_rom_bytes_declared"),
                    "chr_rom_bytes_declared":header.get("chr_rom_bytes_declared"),"chr_tile_count":len(chr_data)//16,
                    "instruction_candidates":summary.get("instruction_candidates"),"gameplay_hypotheses":len(hypotheses),
                    "trace_supplied":bool(trace),"limitations":analysis.get("limitations",[])}
        write_text(target / "manifest.json", json.dumps(manifest, indent=2))
        write_text(target / "README.md", f"# {title} — ROM Development Environment\n\n## Start Here\n\n- title-description.md — title synopsis and research\n- sprites/ — raw CHR tile sheets and CSV catalog\n- game-logic/ — gameplay hypotheses and research notebooks\n- codes/ — candidate disassembly and mapper/register analysis\n- traces/ — optional emulator trace data\n- reports/ — JSON, Markdown, and HTML dashboard\n- manifest.json — input and analysis metadata\n- rom-project/ — immutable baseline copy, editable patch manifest, and rebuilt ROM output\n\n## Rebuild the ROM\n\nEdit rom-project/rom-project.json to enable verified fixed-length patches, then run `python3 {(HERE / "nes_rom_build.py").as_posix()} build --project rom-project/rom-project.json` from this workspace directory. The original input file remains unchanged; a byte-for-byte baseline copy is stored under rom-project/source/original.nes. See `ROM-ROUND-TRIP.md` in the analysis tools directory for details.\n\n## Limits\n\nStatic findings are not recovered source code. Disassembly, mapper mapping, sprite grouping, settings, and gameplay labels may be uncertain. Raw CHR patterns are not necessarily whole sprites. The input ROM was not modified; a byte-for-byte baseline copy is stored under rom-project/source/original.nes for rebuilding.")
        return target
    finally:
        shutil.rmtree(stage, ignore_errors=True)

def main(argv=None):
    p = argparse.ArgumentParser(description="Generate a development-environment-TITLE folder from a local NES ROM.")
    p.add_argument("rom", type=Path)
    p.add_argument("--title", help="friendly title; defaults to ROM filename")
    p.add_argument("--output-root", type=Path, default=Path.cwd(), help="parent directory for output")
    p.add_argument("--trace", type=Path, help="optional emulator-exported JSON or CSV trace")
    p.add_argument("--rules", type=Path, help="optional title-specific game-rules JSON to evaluate and copy")
    p.add_argument("--force", action="store_true", help="replace the matching generated workspace")
    a = p.parse_args(argv)
    try:
        out = generate(a.rom, a.output_root, a.title, a.trace, a.force, a.rules)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    print(f"Workspace created: {out}")
    for label, path in [("Title description",out/"title-description.md"),("Sprite catalog",out/"sprites/sprite-catalog.csv"),
                        ("Game logic",out/"game-logic/gameplay-hypotheses.md"),("Circuit graph",out/"codes/circuit/circuit.dot"),("Action assessment",out/"game-logic/action-assessment.md"),("Disassembly",out/"codes/disassembly/linear-sweep.txt"),
                        ("HTML dashboard",out/"reports/analysis.html"),("Advanced analysis",out/"codes/advanced-analysis.json"),("Game rules",out/"game-logic/game-rules.json")]:
        print(f"{label}: {path}")
    print("Static-analysis findings are hypotheses until validated with runtime evidence.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
