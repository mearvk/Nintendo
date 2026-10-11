# Security and Safe File Handling

- Treat ROMs, archives, patches, manifests, trace logs, and external tool output as untrusted input.
- Validate lengths and offsets with overflow-safe arithmetic before reading or writing.
- Place explicit limits on input sizes, decompression, recursion, and processing time.
- Never overwrite the input file by default; write to a separate output and use atomic replacement only when explicitly requested.
- Reject patches whose expected bytes do not match the source.
- Do not execute code found inside a ROM or archive as part of static analysis.
- Avoid extracting archives into paths supplied by archive entries without traversal and symlink protections.
- Keep credentials and private absolute paths out of logs and manifests.
- Report vulnerabilities privately to the repository maintainers before public disclosure.
