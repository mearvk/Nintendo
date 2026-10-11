# Metadata and Provenance Manifests

Use machine-readable manifests for inventory and reproducible research. A manifest records metadata about a file; it should not contain ROM bytes.

## Recommended fields

- `schema_version`
- `title`, `platform`, `region`, `revision`, `format`
- `relative_path` (local inventory only; do not expose private paths)
- `size_bytes`, `sha256`
- `provenance`, `rights_status`, `redistribution_allowed`
- `analysis_tool`, `tool_version`, `analysis_timestamp`
- `notes` and confidence level for inferred metadata

Use explicit statuses such as `unknown`, `personal_dump`, `authorized`, and `redistribution_approved`. Unknown is not equivalent to permission. Avoid committing manifests that reveal private local directories.
