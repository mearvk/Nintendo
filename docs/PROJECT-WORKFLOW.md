# Project Workflow and UI Requirements

This document specifies the shared workflow for the existing command-line tools and any future desktop launcher.

## Project lifecycle

1. Create a project from a local file the user is entitled to inspect.
2. Record source size and SHA-256 without modifying the source.
3. Detect the container format and report unsupported/ambiguous metadata.
4. Store analysis outputs and logs in a project-specific directory.
5. Let the user review patch scripts and expected-byte guards.
6. Build to a separate output file.
7. Re-open the output, validate structure, and generate before/after hashes.
8. Export a project manifest that contains metadata, not copyrighted payload bytes.

## Launcher requirements

- Recent projects and platform filters.
- Configurable paths for optional external tools.
- Visible tool version, execution status, and logs.
- Clear read-only and write-operation indicators.
- Confirmation before overwriting an existing output.
- No automatic downloading or execution of ROMs, scripts, or external binaries.
- Accessible keyboard navigation and actionable error messages.

The launcher is a planned integration until an actual implementation and UI tests exist.
