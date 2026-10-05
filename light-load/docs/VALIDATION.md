# Validation — the verification series for a new NES edition

How to verify a new edition is structurally sound and runnable before you ship.
The structural checks here are exactly what this repo's tooling automates.

## Stage 1 — structural validation (automated by NTS)

Run the structural reader on your built ROM:

```sh
nts-analyze your-edition.nes            # merit / strategy / components + conditions
nts-analyze your-edition.nes --ruth      # the five-cell verdict diagram
nts-analyze your-edition.nes --declare    # declared PRG/CHR capacity
```

**Pass criteria (the five conditions):**

| condition | must be | meaning |
|---|---|---|
| **play** | ✅ | valid magic, PRG present, not truncated |
| **guarantee** | ✅ | known size, not truncated (a documented trailer is OK) |
| **chapters** | ✅ (if banked) | more than one PRG bank |
| **win** | ✅ (if persistent) | mapped ∧ battery ∧ guarantee |
| **chemistry** | ✅ | CHR source agrees with bank count |

**Rule:** an edition is not "done" until `play`, `chemistry`, and a clean
`guarantee` hold. The C, C++, and Java tools in `nts/` all produce the same
verdict — cross-check across implementations if in doubt.

## Stage 2 — emulator validation (accuracy & behaviour)

| Emulator | Era | Strength |
|---|---|---|
| **Mesen** | modern | Best debugger, cycle-accurate, HD-pack support. Primary dev target. |
| **FCEUX** | long-standing | Lua scripting, debugging, TAS tooling. |
| **Nestopia / puNES** | accuracy refs | Cross-check behaviour against accurate cores. |

**Rules**
- Boot on **at least two** accuracy-focused emulators; divergence usually means
  you are relying on undefined behaviour.
- Use Mesen's debugger to confirm bank-switching, IRQ timing, and PPU writes do
  what you intend.

## Stage 3 — hardware validation (optional, for carts)

- Flash to a compatible board (e.g. **UNROM-512** flash cart) and test on real
  hardware.
- Verify save/battery behaviour physically if you declared WRAM.

## Stage 4 — regression & packaging checks

- Re-run `nts-analyze --tsv` and diff the condition rows against the previous
  build (one row per condition — a stable regression signal).
- Confirm no ROM bytes are committed to the repo (source + assets only; the ROM
  is a build output).
- If shipping an HD pack or patch, validate those artifacts separately.

## The verification series, in one line

**build → `nts-analyze` (structure) → Mesen/FCEUX (behaviour) → optional
hardware → `--tsv` regression diff → package.**
