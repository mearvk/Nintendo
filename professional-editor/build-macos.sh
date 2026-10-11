#!/bin/sh
# Build and test every native C/C++ ROM tool on macOS using Apple's Clang.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
if ! command -v clang >/dev/null 2>&1 || ! command -v make >/dev/null 2>&1; then
  echo "Install Xcode Command Line Tools first: xcode-select --install" >&2
  exit 2
fi
cd "$ROOT"
make release-binaries
make test
echo "PASS: all native C/C++ tools built and passed tests on macOS."
