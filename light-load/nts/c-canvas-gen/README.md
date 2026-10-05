# NTS canvas generator (`nts-canvas`)

Emits a **brand-new, empty NES 2.0 image** of a chosen footprint. It copies
nothing from any existing game — every byte is original (header + zeros). This
is the legitimate "expand the canvas" path: a blank, ownable file you fill with
your own homebrew code.

## Build & run

```sh
cd nts && make        # builds nts-analyze and nts-canvas
./nts-canvas <out> <prg_MB> [chr_MB] [mapper] [--body]
```

- `prg_MB` / `chr_MB` — requested footprint in MB (rounded **up** to the
  nearest NES 2.0-encodable size).
- `mapper` — mapper number to declare (default 5 / MMC5).
- `--body` — also write the full zeroed PRG+CHR body (large!). Without it, only
  the 16-byte header is written: a capacity *declaration* you can inspect with
  `nts-analyze <file> --declare`.

## Examples

```sh
# 200 MB PRG + 32 MB CHR, header only (16-byte capacity declaration)
./nts-canvas canvas-200mb.ineshdr 200 32 5
# inspect:
./nts-analyze canvas-200mb.ineshdr --declare
#   PRG : 224 MB (exponent notation)   <- smallest encodable >= 200 MB
#   CHR : 32 MB  (exponent notation)

# full 256 MiB file on disk (streamed zeros):
./nts-canvas big.canvas.nes 200 32 5 --body
```

## How large sizes are encoded

The NES 2.0 header encodes PRG/CHR size in a 12-bit field. When its high nibble
is `0x0F`, the low byte is **exponent notation**:

```
size = 2^E * (M*2 + 1)      E = 6-bit exponent, M = 2-bit multiplier
```

The generator searches `(E, M)` for the smallest representable size `>=` your
request. So 200 MB rounds up to `2^25 * 7 = 224 MiB` (low byte `0x67`), and
512 MB is `2^29 * 1 = 512 MiB` (low byte, exponent form). The reader
(`c-ines-reader`) decodes the same notation.

## Important: capacity, not playability

> A multi-hundred-MB canvas is a **format-capacity demonstration**, not a
> runnable ROM. The NES 2.0 *header* can declare sizes into the gigabyte range
> on paper, but real mappers, flash carts, and emulators cap out far lower
> (practical homebrew is low-single-digit MB). Nothing here will boot on
> hardware or in an emulator at these sizes — the point is to show the format's
> theoretical envelope and to hand you a blank, ownable canvas.

## Files committed vs. ignored

- `*.ineshdr` — 16-byte original NES 2.0 header declarations: safe to commit.
- `*.canvas.nes` and `nts/*.nes` — full zeroed bodies: **git-ignored** (large,
  and `.nes` is barred by `light-load` policy anyway).
