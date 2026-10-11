# Test Strategy

Run the native and Python test suites documented by each component. Add integration tests when components share file formats.

## Minimum regression matrix

- Valid, invalid, and truncated headers.
- Empty and unusually large inputs with clear size limits.
- Offset/length overflow and out-of-bounds patches.
- Expected-byte guard success and failure.
- Source-file immutability and output-file separation.
- SHA-256 manifest reproducibility.
- Deterministic output from synthetic inputs.
- Cross-platform path and newline behavior.

Use sanitizers where available and fuzz binary parsers with bounded runtime. Tests must not require copyrighted game images. Attach test results to a specific commit and platform; do not treat a configured workflow as a passing run.
