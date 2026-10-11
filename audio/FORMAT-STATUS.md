# Audio Feature Status

| Feature | Status | Required evidence |
|---|---|---|
| NES APU register-write log parser | Planned | Versioned format, bounds tests, synthetic records |
| Channel-state visualization | Planned | Documented register semantics and expected-state tests |
| Waveform export | Planned | Sample format, sample rate, deterministic tests |
| Emulator audio trace adapter | Planned | Emulator/version-specific adapter and fixture traces |

Keep recorded data separate from synthesized approximations. Do not label features implemented until code and tests are present.
