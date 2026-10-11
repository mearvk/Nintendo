# Test Fixtures

This directory is reserved for original, generated, redistributable fixtures. Prefer small synthetic files over commercial ROMs.

## Required fixture categories

- Valid minimal iNES header and small synthetic payload.
- NES 2.0 metadata examples with documented expected interpretation.
- Invalid magic, truncated headers, truncated payloads, and inconsistent size declarations.
- Patch input with matching expected bytes and patch input with deliberately mismatching expected bytes.
- Boundary offsets, empty files, and oversized-length declarations.
- Synthetic tile/nametable and controller-event samples as those parsers are implemented.

Each fixture must have a description, generation method, expected result, and license/provenance. Never copy a copyrighted ROM into this directory to make a test convenient.
