# Circuit-Style Game Logic and Action Analysis

The workbench now exports a candidate control-flow circuit from decoded 6502/2A03 instructions. It treats instructions as nodes and candidate transitions as directed edges. This is a structural analysis layer, not an emulator.

## Outputs

- `circuit-analysis.json`: nodes, edges, branch/fallthrough relationships, calls, loop candidates, adjacent repeated instruction runs, candidate entry nodes, and per-action evidence checks.
- `circuit.dot`: Graphviz DOT representation; render with `dot -Tsvg circuit.dot -o circuit.svg` if Graphviz is installed.
- `game-logic/action-assessment.md`: title-workspace report with precondition/order/repetition/runtime evidence columns and a review method for whether an action is right under stated game rules.
- `reports/analysis.html`: links to the circuit exports.
- `reports/analysis.json`: embeds `circuit_analysis` alongside existing ROM findings.

## Relationships detected

- **Sequence/order:** fallthrough and return-continuation candidates show possible instruction order.
- **Conditional decisions:** branch instructions emit true-target and false/fallthrough edges where targets can be resolved.
- **Calls and continuation:** JSR-like calls are linked to a candidate callee and continuation, with uncertainty noted.
- **Repetition:** backward control-flow edges are marked loop candidates. Adjacent identical instructions are a separate, weaker signal and may be data misdecoded as code.
- **Paths toward a terminator:** reports whether graph traversal can reach RTS, RTI, or BRK candidates. This does not establish a win, level completion, or desirable outcome.
- **Action candidates:** writes, arithmetic/counter operations, calls, and conditional branches are listed with predecessors, successors, repeat membership, and optional trace support.

## Interpreting “right action”

The analyzer intentionally leaves `right_action_assessment.status` as `undetermined`. Opcodes cannot, by themselves, reveal a game's purpose or a moral verdict. To assess whether an action is right *under a specific game rule*, reviewers should establish:

1. **Objective:** identify a documented win condition, progression goal, or rule.
2. **Preconditions:** determine which branch guards, state flags, inputs, or resource thresholds must hold.
3. **Order:** confirm required prior actions and whether order can be changed.
4. **Effects:** trace writes, score/resource changes, collision outcomes, and side effects.
5. **Repetition:** determine whether repetition is intended, bounded, idempotent, or an accidental infinite loop.
6. **Constraints/invariants:** check lives, health, inventory, level state, memory bounds, and any known rule constraints.
7. **Evidence:** correlate the candidate with emulator traces and controlled tests across success, failure, and edge cases.

Only after the objective and constraints are established should the action be labeled provisionally valid, invalid, or unresolved. If “right action” means moral/ethical rightness rather than correct behavior under game rules, the ROM alone is insufficient; that requires an explicit value framework and external context.

## Limitations

Control flow is reconstructed from candidate linear disassembly and approximate bank windows. Data bytes can resemble opcodes; indirect jumps, interrupts, bank switching, and mapper-specific behavior may be unresolved. Loop spans are heuristic. A path to a return opcode is not a path to a game-ending goal. Use emulator traces and mapper-aware execution to strengthen or reject these hypotheses.
