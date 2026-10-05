# Validation — the verification series using our own code

How we verify a new edition is structurally sound before shipping — using
**only our own tools**. No third-party emulators, debuggers, or validators.

> No external dependencies. Every check here runs through code we wrote in this
> repo (`nts/`, in C / C++ / Java).

## Stage 1 — structural validation (our NTS tools)

Run our structural reader on the built ROM:

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
`guarantee` hold. Our C, C++, and Java tools produce the same verdict —
cross-check across our own implementations as an independent confirmation.

## Stage 2 — behavioural validation (our own harness)

Rather than depend on an outside emulator, we verify behaviour with code we
control:

- **Our own execution harness** — a small 6502 interpreter/stepper we write
  that loads the PRG through the mapper model and lets us assert on CPU/PPU/APU
  state after N cycles. (The region model in `nts/` already provides the bank
  layout to drive it.)
- **Deterministic assertions** — golden state checks: after a fixed input
  sequence, specific memory/registers must equal known values we authored.

**Rule:** behaviour is validated against **our** expected-state fixtures, not an
external reference core.

## Stage 3 — hardware validation (optional)

- If we target physical carts, we verify the image against the board's own
  programming spec using our own flashing/verify routine.
- Confirm save/battery behaviour against our own WRAM declaration.

## Stage 4 — regression & packaging checks

- Re-run `nts-analyze --tsv` and diff the condition rows against the previous
  build (one row per condition — a stable regression signal).
- Confirm no ROM bytes are committed (source + data only; the ROM is a build
  output of our code).
- Validate any packaging artifacts (presentation layer, patch) with our own
  checkers.

## The verification series, in one line

**build → `nts-analyze` (structure) → our own execution harness (behaviour) →
optional hardware → `--tsv` regression diff → package.**
