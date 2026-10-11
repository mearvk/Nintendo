# Release Engineering

## Release checklist

- [ ] Confirm working tree changes are reviewed and attributable to the release commit.
- [ ] Build native tools from a clean checkout on supported OS runners.
- [ ] Run unit, integration, malformed-input, and regression tests.
- [ ] Verify documented commands against the produced artifacts.
- [ ] Generate SHA-256 checksums and publish an artifact manifest.
- [ ] Include compiler/toolchain versions and build provenance.
- [ ] Review dependency licenses and known vulnerabilities.
- [ ] Confirm all included ROMs, assets, traces, and patches are authorized for redistribution.
- [ ] Publish release notes describing implemented features and known limitations.
- [ ] Verify downloadable artifacts and checksums after publication.

A successful local build does not establish cross-platform support. A workflow file does not establish a passing CI run. Link results for the exact release commit.
