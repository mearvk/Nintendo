# Audio format status

| Capability | Status | Notes |
|---|---|---|
| APU register-write CSV validation | Implemented | nes_apu_log.py; validates cycle/address/value and emits JSON |
| Waveform synthesis | Not implemented | No APU emulation or sample reconstruction |
| Trace adapters | Planned | Must document emulator and trace schema |

Input columns are cycle,address,value; integers may be decimal or 0x-prefixed. Cycles must be nonnegative and nondecreasing. Accepted registers are $4000-$4013, $4015, and $4017; $4014 is OAM DMA, not an APU register.
