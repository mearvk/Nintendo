# ROM Inventory and Repository Hygiene

The repository has historically contained both `rom/` and `roms/` locations and separate archive collections. Before moving or deleting files, inventory all paths and compare SHA-256 hashes; do not assume same-named files are identical.

## Migration plan

1. Generate a report of every candidate ROM/archive path, file size, and SHA-256.
2. Review duplicates, naming, platform, region, revision, and format.
3. Determine provenance and redistribution status for each item.
4. Choose one canonical location for files whose storage is authorized.
5. Keep a migration map for moved items; do not silently delete ambiguous files.
6. Keep private or unapproved copyrighted files out of published commits.
7. Update scripts, docs, ignore rules, and manifests to reference the canonical location.

## Required safeguards

- Never automatically upload local ROMs.
- Do not treat a checksum as proof of ownership or redistribution rights.
- Unknown provenance must remain flagged for review.
- Prefer metadata and original synthetic fixtures for public CI.
- Make inventory tools read-only unless an explicit migration operation is requested.
