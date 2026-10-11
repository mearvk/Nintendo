# Release checklist

A release candidate requires evidence tied to the exact source commit.

- [ ] Linux, macOS, and Windows native build jobs pass.
- [ ] Python compilation and all ROM-free tests pass.
- [ ] C/C++/Java/SLeeLa parity claims are supported by tests; unverified paths are labelled.
- [ ] Synthetic tests cover malformed headers, truncation, bank boundaries, patch bounds, and byte-preserving output.
- [ ] ROM manifests verify SHA-256, size, provenance, rights status, and redistribution policy.
- [ ] Artifacts have SHA-256 checksums and a source commit identifier.
- [ ] Release notes list supported platforms, limitations, and reproducible build commands.
- [ ] No unauthorized ROMs, extracted assets, save files, or copyrighted patch payloads are distributed.
- [ ] Dependencies and workflows use least-privilege permissions and reviewed versions.
- [ ] No emulation or verified-control-flow claims without implemented, tested capabilities.

Generate checksums beside artifacts with sha256sum on Linux/macOS or Get-FileHash -Algorithm SHA256 on Windows.
