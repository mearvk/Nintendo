# Advanced NES Analysis: Rules, Mapper State, Memory, and Scenes

This layer adds evidence-oriented research tools to each generated title workspace. It is intentionally not a full NES emulator.

## Editable game rules

The workspace generator writes `game-logic/game-rules.json`. Replace the example entry with the title's verified objective, constraints, action addresses, and relative-order requirements. Example:

```json
{
  "schema_version": "1.0",
  "title": "Example Title",
  "objective": "Reach the exit while retaining at least one life",
  "constraints": ["Do not trigger the loss state"],
  "actions": [
    {
      "address": "0x8120",
      "label": "evaluate exit condition",
      "verdict": "required",
      "rationale": "Verified against controlled gameplay traces",
      "must_precede": ["0x8180"],
      "must_follow": [],
      "max_visits": null
    }
  ],
  "state_assertions": [],
  "notes": ["Keep every address and interpretation tied to evidence."]
}
```

Allowed verdicts are `required`, `allowed`, and `forbidden`. Ordering checks use first occurrences in the supplied trace and therefore need adequate trace coverage. Re-run with:

```sh
python3 analysis/nes_dev_environment.py game.nes --rules development-environment-TITLE/game-logic/game-rules.json --trace trace.json --force
```

The workspace includes `game-logic/rules-evaluation.json`, a readable Markdown report, and `codes/advanced-analysis.json`. Results are `consistent-with-declared-rule`, `conflicts-with-declared-rule`, or `unresolved`. A static instruction being present is not proof it ran; a trace missing an event is not proof the event never happens. The rules evaluator does not infer moral correctness or claim a complete win/loss proof.

## Mapper-state profiles

The module reports candidate PRG windows for NROM (mapper 0), MMC1 (mapper 1), UxROM (mapper 2), and CNROM (mapper 3). UxROM's switchable lower window and fixed upper window are recorded explicitly. MMC1 serial/control state and CNROM CHR bank state are documented as unexecuted state models. This is groundwork for stateful mapping, not an emulator: no CPU instruction or mapper write is executed.

## RAM and hardware I/O classification

Addresses are categorized into CPU RAM/mirrors, PPU registers, APU/I/O, controller-related registers, expansion space, cartridge PRG-RAM/save-RAM candidates, and mapper-controlled ROM. The classification is based on the NES CPU address map. Cartridge-specific RAM presence, write protection, mapper banking, and semantic variable names remain evidence-dependent.

## Palette and scene reconstruction

The scene helper accepts PPU nametable, attribute-table, and palette snapshots. It builds a 32×30 tile-cell color-index grid when the required snapshots exist. It does not claim full pixel-accurate rendering without CHR/CHR-RAM pattern bytes, correct palette mirroring, and synchronized PPU state. Sprite reconstruction additionally needs OAM and sprite-pattern data; OAM positions alone are not a rendered frame.

## Emulator adapters, stepping, and replay

Trace normalization accepts a JSON event list or an object containing `events`/ `trace`, plus common PC/frame/cycle/input field aliases. It emits a normalized trace and a frame/cycle-associated input event list when input data is available.

The current tool does **not** start an emulator, control single-step, or inject controller input. Those actions require a live emulator's documented debug protocol and explicit user configuration. Adapter work should be isolated behind a provider interface and must record emulator/version, ROM hash, region, mapper, timing, input encoding, and capture settings. Never silently treat an imported trace as synchronized or complete.

## Validation procedure

1. Start with a clean boot trace, then separately capture title/menu, active play, pause, death/loss, win/transition, and reset.
2. Keep the same ROM hash and emulator configuration across comparison traces.
3. Annotate controller changes and frame/cycle values.
4. Validate mapper register writes and bank identity before assigning CPU addresses to physical ROM offsets.
5. Record RAM/I/O reads and writes with address, width, PC, old/new values if available, and scene context.
6. Compare expected state changes against the editable game rules.
7. Keep unsupported or ambiguous findings unresolved rather than inventing game logic.
