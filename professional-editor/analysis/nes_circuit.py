"""Circuit-style control-flow and action-semantics analysis for NES disassembly.

This is an evidence ledger over candidate instructions, not a CPU emulator.
"""
from collections import Counter, defaultdict, deque

TERMINATORS = {"RTS", "RTI", "BRK"}
CONDITIONAL = {"BPL", "BMI", "BVC", "BVS", "BCC", "BCS", "BNE", "BEQ"}
CALLS = {"JSR"}
UNCONDITIONAL_JUMPS = {"JMP"}
WRITES = {"STA", "STX", "STY", "INC", "DEC", "ASL", "LSR", "ROL", "ROR"}
READS = {"LDA", "LDX", "LDY", "BIT", "CMP", "CPX", "CPY"}
ARITHMETIC = {"ADC", "SBC", "INC", "DEC", "ASL", "LSR", "ROL", "ROR"}
GOAL_LIKE = {"RTS", "RTI", "BRK"}


def _addr(value):
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        try:
            return int(value, 16) if value.lower().startswith("0x") else int(value)
        except ValueError:
            return None
    return None


def _target(ins):
    return _addr(ins.get("target_cpu_address"))


def _cpu(ins):
    return _addr(ins.get("cpu_address_candidate"))


def _offset(ins):
    return _addr(ins.get("file_offset"))


def analyze_circuit(instructions, trace=None, max_nodes=12000):
    """Build a conservative control-flow circuit with sequence/loop/path hints."""
    items = [dict(x) for x in instructions if isinstance(x, dict) and x.get("mnemonic")]
    items = items[:max_nodes]
    by_cpu = defaultdict(list)
    for i, ins in enumerate(items):
        addr = _cpu(ins)
        if addr is not None:
            by_cpu[addr].append(i)
    edges = []
    edge_keys = set()
    def add(src, dst, kind, confidence="medium"):
        if dst is None or dst < 0 or dst >= len(items) or src == dst and kind == "fallthrough":
            return
        key = (src, dst, kind)
        if key not in edge_keys:
            edge_keys.add(key)
            edges.append({"from": src, "to": dst, "kind": kind, "confidence": confidence})
    for idx, ins in enumerate(items):
        m = ins.get("mnemonic", "")
        nxt = idx + 1 if idx + 1 < len(items) else None
        target = _target(ins)
        candidates = by_cpu.get(target, []) if target is not None else []
        target_idx = candidates[0] if candidates else None
        if m in TERMINATORS or ins.get("flow") == "return" or m == ".byte":
            continue
        if m in CONDITIONAL or ins.get("flow") == "conditional_branch":
            add(idx, target_idx, "condition-true" if target_idx is not None else "condition-target-unresolved",
                "medium" if target_idx is not None else "low")
            add(idx, nxt, "condition-false/fallthrough")
        elif m in CALLS or ins.get("flow") == "call":
            add(idx, target_idx, "call-target" if target_idx is not None else "call-target-unresolved",
                "medium" if target_idx is not None else "low")
            add(idx, nxt, "return-continuation", "low")
        elif m in UNCONDITIONAL_JUMPS or ins.get("flow") == "jump":
            if target_idx is not None:
                add(idx, target_idx, "jump")
            else:
                add(idx, None, "jump-target-unresolved")
        else:
            add(idx, nxt, "sequence")
    adjacency = defaultdict(list)
    reverse = defaultdict(list)
    for e in edges:
        adjacency[e["from"]].append(e["to"])
        reverse[e["to"]].append(e["from"])
    back_edges = [e for e in edges if e["to"] <= e["from"]]
    loop_nodes = set()
    for e in back_edges:
        # Approximate a loop body as the source-to-back-edge span. Address/bank
        # layout and instruction decoding can make this conservative heuristic wrong.
        loop_nodes.update(range(e["to"], e["from"] + 1))
        e["loop_candidate"] = True
    node_records = []
    for i, ins in enumerate(items):
        m = ins.get("mnemonic", "")
        node_records.append({
            "id": i, "address": ins.get("cpu_address_candidate"),
            "file_offset": ins.get("file_offset"), "instruction": m,
            "operand": ins.get("operand", ""), "bytes": ins.get("bytes", []),
            "role_candidates": (["state-write-or-side-effect"] if m in WRITES else []) +
                               (["state-read-or-condition"] if m in READS else []) +
                               (["arithmetic-or-counter-update"] if m in ARITHMETIC else []) +
                               (["call"] if m in CALLS else []) +
                               (["conditional-decision"] if m in CONDITIONAL else []) +
                               (["candidate-terminal-or-return"] if m in TERMINATORS else []),
            "in_loop_candidate": i in loop_nodes,
        })
    components = []
    visited = set()
    for e in back_edges:
        lo, hi = sorted((e["from"], e["to"]))
        components.append({"type": "repeat-or-loop", "entry_node": lo, "back_edge": [e["from"], e["to"]],
                           "node_span": [lo, hi], "estimated_body_nodes": hi-lo+1,
                           "evidence": "control-flow edge points backward in the decoded listing",
                           "confidence": "medium" if e["kind"] != "condition-target-unresolved" else "low"})
    # Consecutive identical instructions and operands can reveal repeated operations,
    # but are weaker than a proven loop and are explicitly labelled separately.
    repeated_runs = []
    i = 0
    while i < len(items):
        signature = (items[i].get("mnemonic"), items[i].get("operand"))
        j = i + 1
        while j < len(items) and (items[j].get("mnemonic"), items[j].get("operand")) == signature:
            j += 1
        if j-i >= 2:
            repeated_runs.append({"type":"adjacent-repetition","start_node":i,"end_node":j-1,
                                  "count":j-i,"instruction":signature[0],"operand":signature[1],
                                  "confidence":"high","note":"Repeated bytes/instructions in listing; may be data misdecoded as code."})
        i = j
    indegree = Counter()
    for e in edges:
        indegree[e["to"]] += 1
    entries = [i for i in range(len(items)) if indegree[i] == 0][:100]
    if not entries and items:
        entries = [0]
    # Reachability from candidate entries and shortest route to terminal-like instructions.
    reachable = set(entries)
    q = deque(entries)
    while q:
        cur = q.popleft()
        for nxt in adjacency[cur]:
            if nxt not in reachable:
                reachable.add(nxt)
                q.append(nxt)
    terminals = [i for i, x in enumerate(items) if x.get("mnemonic") in GOAL_LIKE]
    reverse_reachable = set(terminals)
    q = deque(terminals)
    while q:
        cur = q.popleft()
        for prev in reverse[cur]:
            if prev not in reverse_reachable:
                reverse_reachable.add(prev)
                q.append(prev)
    sequences = []
    for e in edges:
        if e["kind"] in ("sequence", "return-continuation", "condition-false/fallthrough"):
            sequences.append({"from_node":e["from"],"to_node":e["to"],"relationship":"candidate-order",
                              "confidence":e["confidence"],"evidence":e["kind"]})
    actions = []
    for i, ins in enumerate(items):
        m = ins.get("mnemonic", "")
        if m not in WRITES and m not in ARITHMETIC and m not in CALLS and m not in CONDITIONAL:
            continue
        predecessors = reverse.get(i, [])
        successors = adjacency.get(i, [])
        reachable_terminal = any(n in reverse_reachable for n in successors)
        category = "state-change candidate" if m in WRITES else "decision candidate" if m in CONDITIONAL else "procedure call" if m in CALLS else "numeric/state transition candidate"
        checks = []
        if predecessors:
            checks.append({"criterion":"has incoming control-flow evidence","status":"supported-by-static-graph"})
        else:
            checks.append({"criterion":"has incoming control-flow evidence","status":"entry-or-unresolved"})
        checks.append({"criterion":"can reach return/interrupt-return/BRK candidate",
                       "status":"candidate-path-found" if reachable_terminal else "not-established"})
        checks.append({"criterion":"observed at runtime","status":"supported-by-trace" if trace and _cpu(ins) is not None and any(
            _addr(p.get("pc")) == _cpu(ins) for p in trace.get("events",[]) if isinstance(p,dict)) else "no-matching-trace-evidence"})
        actions.append({"node":i,"address":ins.get("cpu_address_candidate"),"instruction":m,
                        "operand":ins.get("operand",""),"category":category,
                        "predecessor_nodes":predecessors[:20],"successor_nodes":successors[:20],
                        "in_repeat_candidate":i in loop_nodes,"checks":checks,
                        "right_action_assessment":{"status":"undetermined",
                            "reason":"Static opcode/control flow cannot establish player intent, game rules, or moral correctness.",
                            "evaluate":["preconditions and branch guards","whether the state change advances a documented game objective",
                                        "effects on player/enemy/resources and side effects","invariants and safety constraints",
                                        "repeatability/idempotence and failure paths","runtime trace or controlled test evidence"]}})
    return {
        "schema_version":"1.0",
        "model":"candidate-instruction control-flow circuit",
        "node_count":len(node_records),"edge_count":len(edges),
        "nodes":node_records,"edges":edges,
        "candidate_entries":entries,
        "reachable_from_candidate_entries":len(reachable),
        "terminal_candidates":[{"node":i,"address":items[i].get("cpu_address_candidate"),"instruction":items[i].get("mnemonic")} for i in terminals],
        "candidate_paths_toward_terminal_count":sum(1 for i in range(len(items)) if i in reverse_reachable),
        "loops":components,"repeated_instruction_runs":repeated_runs,
        "sequence_edges":sequences[:20000],"action_assessments":actions[:12000],
        "interpretation_policy":{
            "right_action":"A testable engineering/game-rule judgment, not an intrinsic moral verdict. Establish the intended objective and constraints first; then compare preconditions, effects, order, repetition, and observed outcomes.",
            "static_only":"All graph links depend on candidate disassembly and approximate bank mapping. Indirect jumps, interrupts, data decoded as code, and bank switches can alter actual execution.",
            "path_semantics":"A path toward RTS/RTI/BRK is only a path toward a return/interrupt-return/break opcode candidate, not proof of reaching a win state or desirable outcome."
        }
    }


def to_dot(circuit, limit=2500):
    """Export Graphviz DOT with semantic edge labels and repeat-node styling."""
    nodes = circuit.get("nodes", [])[:limit]
    allowed = {n["id"] for n in nodes}
    lines = ["digraph NES_Circuit {", "  rankdir=LR;", '  graph [label="NES candidate control-flow circuit", labelloc=t];',
             '  node [shape=box, fontname="monospace"];']
    for node in nodes:
        label = f'{node.get("address","?")}\\n{node.get("instruction","?")} {node.get("operand","")}'.replace('"', '\\"')
        attrs = 'style="filled", fillcolor="lightyellow"' if node.get("in_loop_candidate") else ""
        lines.append(f'  n{node["id"]} [label="{label}"{", "+attrs if attrs else ""}];')
    for edge in circuit.get("edges", []):
        if edge["from"] in allowed and edge["to"] in allowed:
            color = "red" if edge.get("loop_candidate") else "black"
            label = edge.get("kind", "")
            lines.append(f'  n{edge["from"]} -> n{edge["to"]} [label="{label}", color="{color}"];')
    if len(circuit.get("nodes", [])) > limit:
        lines.append(f'  // truncated: showing first {limit} of {len(circuit["nodes"])} nodes')
    lines.append("}")
    return "\n".join(lines) + "\n"
