# Nintendo Studio Development Roadmap

This roadmap turns the repository's existing NES editor and analysis tools into a maintainable, cross-platform retro-development workbench. It does not imply that planned features are already implemented.

## Priorities

1. **Inventory and provenance** — establish a canonical metadata manifest, calculate SHA-256 hashes, document provenance and redistribution status, and audit all existing binary archives before redistribution.
2. **Stabilize the editor** — preserve the original ROM, require explicit output paths for modifications, validate patch bounds and expected bytes, and test malformed/truncated inputs.
3. **Hardware-aware analysis** — document platform architecture and distinguish confirmed facts from inferred mapper, bank, and control-flow hypotheses.
4. **Graphics and audio tooling** — add format-aware viewers/converters only with documented input formats and round-trip tests.
5. **Emulator integration** — import traces and debug symbols through adapters; do not claim emulation unless an actual emulator backend is integrated.
6. **Unified project workflow** — recent projects, tool configuration, logs, platform filters, and explicit read-only versus write operations.
7. **Release engineering** — reproducible builds, OS-specific artifacts, checksums, software bill of materials where feasible, and release notes.

## Proposed layout

- `docs/hardware/` — platform architecture and reference notes.
- `emulation/` — emulator adapters and trace formats.
- `graphics/` — tile, sprite, palette, and nametable tooling.
- `audio/` — audio hardware references and analysis adapters.
- `input/` — controller protocols and mapping tests.
- `fixtures/` — generated, original test files only.
- `manifests/` — ROM metadata, hashes, provenance, and format declarations.
- `tests/` — cross-tool regression and integration tests.
- `releases/` — release checklist and artifact provenance.

## Definition of done

- Linux, macOS, and Windows builds are exercised by CI where runners and dependencies permit.
- Synthetic fixtures test malformed headers, truncation, offset boundaries, and byte-preserving operations.
- Every write operation is bounded, rejects unexpected source bytes, and writes to a separate output by default.
- Test results are tied to an exact commit; workflow configuration alone is not proof of a passing run.
- No commercial ROM, save, extracted asset, or copyrighted patch payload is added without documented authorization.
- Documentation labels features as implemented, experimental, or planned and records known limitations.
