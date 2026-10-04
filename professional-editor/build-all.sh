#!/bin/sh
# build-all.sh -- build the C, C++, and Java editors. SLeeLa is interpreted
# (run with the `sleela` runtime); nothing to build for it here.
set -eu
ROOT="$(cd "$(dirname "$0")" && pwd)"

echo "== C =="
make -C "$ROOT/c"

echo "== C++ =="
make -C "$ROOT/cpp"

echo "== Java =="
# Compile with javac (works with any JDK >= 21). Use Gradle instead with:
#   cd java && gradle build   (requires a JDK 21 toolchain)
mkdir -p "$ROOT/java/build/classes"
javac -d "$ROOT/java/build/classes" $(find "$ROOT/java/src" -name '*.java')
echo "Built: $ROOT/java/build/classes"
echo "  run: java -cp java/build/classes com.mearvk.nintendo.editor.NesEditCli"

echo "== SLeeLa =="
echo "SLeeLa model is interpreted: sleela run $ROOT/sleela/NesEditModel.sleela"

echo "Done."
