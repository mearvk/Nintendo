# Metadata and provenance manifests

Create a sidecar without modifying the source image:

```sh
python tools/rom_manifest.py create owned-or-authorized.nes manifests/local-image.json --title "My Dump" --platform NES --format iNES --provenance "Personal dump" --rights-status personal_dump
python tools/rom_manifest.py verify manifests/local-image.json owned-or-authorized.nes
```

The tool streams SHA-256 calculation, records byte size and UTC timestamp, and verifies both later. Rights default to unknown and redistribution is disabled. Only enable redistribution when documented rights support it; this tool is not legal advice. Do not commit private local paths or confidential acquisition details.

Recommended metadata includes schema version, title/platform/region/revision/format, size and SHA-256, provenance and rights status, redistribution flag, analysis tool/version/timestamp, notes, and confidence for inferred metadata. Unknown is not permission.
