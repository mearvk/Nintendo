#!/usr/bin/env python3
"""Advanced NES research helpers: declared rules, mapper state, RAM/I/O and scene snapshots.

This module deliberately analyzes supplied metadata/traces; it does not emulate a CPU or launch an emulator.
"""
from __future__ import annotations
import json
from pathlib import Path

SCHEMA_VERSION = "1.0"
NES_PALETTE_RGB = [
    (84,84,84),(0,30,116),(8,16,144),(48,0,136),(68,0,100),(92,0,48),(84,4,0),(60,24,0),
    (32,42,0),(8,58,0),(0,64,0),(0,60,0),(0,50,60),(0,0,0),(0,0,0),(0,0,0),
    (152,150,152),(8,76,196),(48,50,236),(92,30,228),(136,20,176),(160,20,100),(152,34,32),(120,60,0),
    (84,90,0),(40,114,0),(8,124,0),(0,118,40),(0,102,120),(0,0,0),(0,0,0),(0,0,0),
    (236,238,236),(76,154,236),(120,124,236),(176,98,236),(228,84,236),(236,88,180),(236,106,100),(212,136,32),
    (160,170,0),(116,196,0),(76,208,32),(56,204,108),(56,180,204),(60,60,60),(0,0,0),(0,0,0),
    (236,238,236),(168,204,236),(188,188,236),(212,178,236),(236,174,236),(236,174,212),(236,180,176),(228,196,144),
    (204,210,120),(180,222,120),(168,226,144),(152,226,180),(160,214,228),(160,162,160),(0,0,0),(0,0,0)
]

def rules_template(title=""):
    return {
        "schema_version": SCHEMA_VERSION,
        "title": title,
        "objective": "",
        "constraints": [],
        "actions": [
            {
                "address": "0x8000",
                "label": "Replace this example with a verified action address",
                "verdict": "allowed",
                "rationale": "Template only; remove or edit this entry.",
                "must_precede": [],
                "must_follow": [],
                "max_visits": None
            }
        ],
        "state_assertions": [
            {"address": "0x0000", "operator": "equals", "value": 0, "label": "example only"}
        ],
        "notes": [
            "Addresses and meanings must be verified for this title and mapper.",
            "A ruleset is a research hypothesis, not proof that a playthrough is correct."
        ]
    }

def _addr(value):
    try:
        return int(str(value), 0)
    except (ValueError, TypeError):
        return None

def evaluate_rules(rules, instructions, trace=None):
    """Evaluate explicit rules against available static instruction and trace evidence."""
    issues = []
    by_addr = {}
    for ins in instructions or []:
        addr = _addr(ins.get("cpu_address_candidate", ins.get("address")))
        if addr is not None:
            by_addr.setdefault(addr, []).append(ins)
    events = (trace or {}).get("events", []) if isinstance(trace, dict) else []
    observed_pc = []
    for event in events:
        pc = _addr(event.get("pc"))
        if pc is not None:
            observed_pc.append(pc)
    actions = rules.get("actions", []) if isinstance(rules, dict) else []
    declared = {}
    for action in actions:
        addr = _addr(action.get("address"))
        label = str(action.get("label") or (f"0x{addr:04X}" if addr is not None else "invalid-address"))
        status = "unresolved"
        evidence = []
        if addr is None:
            status = "unresolved"
            evidence.append("Action address is missing or not an integer literal.")
        elif addr not in by_addr:
            evidence.append("No disassembled instruction at this address; bank mapping or code/data classification may be wrong.")
        else:
            evidence.append(f"Static instruction candidate count: {len(by_addr[addr])}.")
            status = "consistent-with-declared-rule" if action.get("verdict") in ("allowed", "required") else "unresolved"
        if addr is not None and observed_pc:
            count = observed_pc.count(addr)
            evidence.append(f"Supplied trace PC hits: {count}.")
            if count:
                status = "consistent-with-declared-rule" if status != "conflicts-with-declared-rule" else status
            elif action.get("verdict") == "required":
                status = "unresolved"
                evidence.append("Required action was not observed in this trace; trace coverage may be incomplete.")
        elif action.get("verdict") == "forbidden":
            status = "unresolved"
            evidence.append("Static presence alone does not prove execution; a forbidden action needs runtime path evidence.")
        declared.setdefault(addr, []).append(action)
        issues.append({"address": f"0x{addr:04X}" if addr is not None else None, "label": label,
            "verdict": action.get("verdict", "unspecified"), "status": status, "evidence": evidence})
    # Relative order is checked only from a trace with explicit PC events.
    if observed_pc:
        positions = {}
        for i, pc in enumerate(observed_pc):
            positions.setdefault(pc, []).append(i)
        for action in actions:
            addr = _addr(action.get("address"))
            if addr is None: continue
            for rel_key, relation in (("must_precede", "precedes"), ("must_follow", "follows")):
                for other_raw in action.get(rel_key, []):
                    other = _addr(other_raw)
                    a_pos, b_pos = positions.get(addr, []), positions.get(other, [])
                    if not a_pos or not b_pos:
                        result = "unresolved"
                        why = "One or both addresses were not observed in the supplied trace."
                    elif relation == "precedes":
                        result = "consistent-with-declared-rule" if min(a_pos) < min(b_pos) else "conflicts-with-declared-rule"
                        why = "Observed first-occurrence order in the supplied trace."
                    else:
                        result = "consistent-with-declared-rule" if min(a_pos) > min(b_pos) else "conflicts-with-declared-rule"
                        why = "Observed first-occurrence order in the supplied trace."
                    issues.append({"address": f"0x{addr:04X}", "label": action.get("label", ""),
                        "criterion": f"{relation} {other_raw}", "status": result, "evidence": [why]})
    return {"schema_version": SCHEMA_VERSION, "title": rules.get("title", ""),
        "objective": rules.get("objective", ""), "constraint_count": len(rules.get("constraints", [])),
        "action_count": len(actions), "checks": issues,
        "summary": {"consistent": sum(x.get("status") == "consistent-with-declared-rule" for x in issues),
                    "conflicts": sum(x.get("status") == "conflicts-with-declared-rule" for x in issues),
                    "unresolved": sum(x.get("status") == "unresolved" for x in issues)},
        "limitations": ["Static presence is not proof of execution.", "Trace order reflects only the supplied capture window.",
            "No automatic claim of win, loss, correctness, or moral rightness is made."]}

def classify_address(address, mapper=None):
    """Return a candidate NES address region; cartridge-specific ranges remain mapper-dependent."""
    a = _addr(address)
    if a is None or not 0 <= a <= 0xFFFF:
        return {"region": "unknown", "confidence": "low", "note": "Address is not a valid 16-bit integer."}
    if a < 0x2000:
        return {"region": "CPU RAM (2 KiB mirrored)", "canonical_address": f"0x{a % 0x800:04X}", "confidence": "high"}
    if a < 0x4000:
        reg = 0x2000 + ((a - 0x2000) % 8)
        return {"region": "PPU register (mirrored)", "canonical_address": f"0x{reg:04X}", "confidence": "high"}
    if a == 0x4014:
        return {"region": "OAM DMA register", "confidence": "high"}
    if a in (0x4016, 0x4017):
        return {"region": "controller strobe/input or APU frame-counter register", "confidence": "medium"}
    if 0x4000 <= a <= 0x4017:
        return {"region": "APU/audio or I/O register", "confidence": "high"}
    if 0x4018 <= a <= 0x401F:
        return {"region": "test-mode I/O (normally disabled)", "confidence": "medium"}
    if 0x4020 <= a <= 0x5FFF:
        return {"region": "expansion device / mapper-specific I/O", "confidence": "medium"}
    if 0x6000 <= a <= 0x7FFF:
        return {"region": "cartridge PRG-RAM / save RAM candidate", "confidence": "medium",
                "note": "Presence and enablement depend on cartridge and mapper."}
    return {"region": "cartridge PRG-ROM / mapper-controlled window", "confidence": "medium",
            "note": "Physical bank depends on mapper state and may change during execution."}

def mapper_profile(mapper, prg_size):
    """Describe simplified PRG windows for common mappers; no register writes are executed."""
    if mapper == 0:
        return {"mapper": mapper, "name": "NROM", "supported": True,
            "windows": [{"cpu_start": 0x8000, "cpu_end": 0xFFFF, "banking": "fixed",
                         "bank_count": 1 if prg_size <= 16384 else 2}],
            "state_model": "fixed mapping"}
    if mapper == 2:
        count = max(1, prg_size // 16384)
        return {"mapper": mapper, "name": "UxROM", "supported": True,
            "windows": [{"cpu_start": 0x8000, "cpu_end": 0xBFFF, "banking": "switchable-16KiB"},
                        {"cpu_start": 0xC000, "cpu_end": 0xFFFF, "banking": "fixed-last-16KiB"}],
            "bank_count": count, "state_model": "candidate switchable low bank; high bank fixed to last"}
    if mapper == 3:
        count = max(1, prg_size // 16384)
        return {"mapper": mapper, "name": "CNROM", "supported": True,
            "windows": [{"cpu_start": 0x8000, "cpu_end": 0xFFFF, "banking": "fixed-PRG"}],
            "prg_bank_count": count, "state_model": "PRG fixed; CHR bank switching requires mapper register state"}
    if mapper == 1:
        return {"mapper": mapper, "name": "MMC1", "supported": True,
            "windows": [{"cpu_start": 0x8000, "cpu_end": 0xFFFF, "banking": "serial-register-controlled"}],
            "state_model": "MMC1 shift/control state is not emulated; annotate runtime bank writes"}
    return {"mapper": mapper, "name": "unknown", "supported": False, "windows": [],
            "state_model": "No mapper-specific register state model is available."}

def palette_scene_snapshot(nametable, attributes, palette, chr_data=None, width=32, height=30):
    """Reconstruct a 32x30 background color-index grid from supplied PPU snapshots."""
    nt = list(nametable or [])
    attr = list(attributes or [])
    pal = list(palette or [])
    if len(nt) < width * height or len(pal) < 16:
        return {"supported": False, "reason": "Need at least 960 nametable bytes and 16 palette bytes."}
    grid = []
    for y in range(height):
        row = []
        for x in range(width):
            tile = nt[y * width + x]
            ai = (y // 4) * 8 + (x // 4)
            if ai < len(attr):
                shift = ((y % 4) // 2) * 4 + ((x % 4) // 2) * 2
                subpalette = (attr[ai] >> shift) & 3
            else:
                subpalette = 0
            color_slot = 0 if (tile == 0) else subpalette * 4 + 1
            color_index = pal[color_slot] & 0x3F
            row.append({"tile": tile, "subpalette": subpalette, "palette_slot": color_slot,
                        "color_index": color_index, "rgb": NES_PALETTE_RGB[color_index]})
        grid.append(row)
    return {"supported": True, "width": width, "height": height, "grid": grid,
        "chr_tile_pixels_included": False,
        "note": "Color-index preview of tile cells only. Full pixel rendering requires CHR/CHR-RAM pattern data, attribute bytes, correct palette mirroring and PPU state."}

def normalize_trace_events(trace):
    """Normalize common trace aliases without discarding source fields."""
    if isinstance(trace, dict):
        events = trace.get("events", trace.get("trace", []))
        adapter = trace.get("adapter", trace.get("source", "generic-json"))
    elif isinstance(trace, list):
        events, adapter = trace, "generic-json"
    else:
        return {"adapter": "unknown", "events": [], "warnings": ["Trace root must be an event list or object."]}
    out, warnings = [], []
    for index, event in enumerate(events):
        if not isinstance(event, dict): continue
        item = dict(event)
        aliases = {"program_counter": "pc", "PC": "pc", "address_pc": "pc", "frame_number": "frame",
                   "cycle_count": "cycle", "controller": "input", "buttons": "input"}
        for source, target in aliases.items():
            if target not in item and source in item: item[target] = item[source]
        for key in ("pc", "frame", "cycle", "address", "value"):
            if key in item and isinstance(item[key], str):
                try: item[key] = int(item[key], 0)
                except ValueError: pass
        item.setdefault("event_index", index)
        out.append(item)
    if not out: warnings.append("No dictionary events found; verify the emulator export format.")
    return {"adapter": str(adapter), "event_count": len(out), "events": out,
            "warnings": warnings, "stepping": {"supported": False, "note": "This analysis layer normalizes traces; CPU single-step control requires a live emulator adapter."},
            "input_replay": {"supported": bool(any("input" in e for e in out)),
                "events": [{"frame": e.get("frame"), "cycle": e.get("cycle"), "input": e.get("input")}
                           for e in out if "input" in e],
                "note": "Replay is a trace-derived event list, not an emulator command stream. Timing and button encoding must be confirmed for the source emulator."}}

def analyze_advanced(mapper, prg_size, instructions, trace=None, rules=None):
    events = normalize_trace_events(trace) if trace is not None else normalize_trace_events([])
    return {
        "schema_version": SCHEMA_VERSION,
        "memory_classification": [
            {"address": f"0x{_addr(i.get('operand','')):04X}", **classify_address(i.get("operand"))}
            for i in (instructions or []) if _addr(i.get("operand")) is not None
        ][:10000],
        "mapper_state_model": mapper_profile(mapper, prg_size),
        "normalized_trace": {k:v for k,v in events.items() if k != "events"},
        "trace_events_normalized": events.get("events", [])[:10000],
        "rules_evaluation": evaluate_rules(rules or rules_template(), instructions, trace if isinstance(trace, dict) else None),
        "palette_scene": {"available": False, "reason": "Requires explicit nametable, attribute-table, palette and CHR/CHR-RAM snapshots from a compatible emulator trace."},
        "limitations": [
            "Mapper profiles describe candidate bank windows; they do not execute mapper registers or emulate bank switching.",
            "Scene reconstruction needs synchronized PPU snapshots; a nametable alone cannot reconstruct sprites or full pixels.",
            "Single-step and input replay require a live emulator adapter and emulator-specific protocol.",
            "Rules are evaluated only against declared addresses and supplied trace evidence; unresolved means insufficient evidence."
        ]
    }

def load_rules(path):
    path = Path(path)
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or data.get("schema_version") != SCHEMA_VERSION:
        raise ValueError(f"Rules file must be an object with schema_version {SCHEMA_VERSION}")
    if not isinstance(data.get("actions", []), list) or not isinstance(data.get("constraints", []), list):
        raise ValueError("Rules actions and constraints must be arrays")
    for action in data.get("actions", []):
        if action.get("verdict", "allowed") not in ("required", "allowed", "forbidden"):
            raise ValueError("Action verdict must be required, allowed, or forbidden")
    return data
