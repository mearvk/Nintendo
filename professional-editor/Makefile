# Native and Python ROM analysis tools plus per-title workspace generator
CC ?= cc
CXX ?= c++
CFLAGS ?= -O2 -std=c99 -Wall -Wextra -Wpedantic
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
BUILD_DIR ?= build
ifeq ($(OS),Windows_NT)
EXEEXT ?= .exe
else
EXEEXT ?=
endif

.PHONY: all native analysis-native c cpp disasm-native cli release-binaries test test-python test-native test-analysis-native test-cli dev-env dev-env-help rom-build-help rom-project rom-build example-rom example-inspect examples-help clean help
all: native analysis-native cli
native: c cpp disasm-native
c: $(BUILD_DIR)/nes-native-c$(EXEEXT) $(BUILD_DIR)/nes-rom-build-c$(EXEEXT)
cpp: $(BUILD_DIR)/nes-native-cpp$(EXEEXT) $(BUILD_DIR)/nes-rom-build-cpp$(EXEEXT)
disasm-native: $(BUILD_DIR)/nes-disasm-c$(EXEEXT) $(BUILD_DIR)/nes-disasm-cpp$(EXEEXT)
analysis-native: $(BUILD_DIR)/rom-manifest-c$(EXEEXT) $(BUILD_DIR)/rom-manifest-cpp$(EXEEXT) $(BUILD_DIR)/nes-chr-c$(EXEEXT) $(BUILD_DIR)/nes-chr-cpp$(EXEEXT) $(BUILD_DIR)/nes-apu-c$(EXEEXT) $(BUILD_DIR)/nes-apu-cpp$(EXEEXT)
cli: $(BUILD_DIR)/nes-dev-env
$(BUILD_DIR):
	mkdir -p "$@"
$(BUILD_DIR)/nes-native-c$(EXEEXT): analysis/nes_native.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-native-cpp$(EXEEXT): analysis/nes_native.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-rom-build-c$(EXEEXT): analysis/nes_rom_build.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-rom-build-cpp$(EXEEXT): analysis/nes_rom_build.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-disasm-c$(EXEEXT): analysis/nes_disasm_native.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-disasm-cpp$(EXEEXT): analysis/nes_disasm_native.cpp analysis/nes_disasm_native.c | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"

$(BUILD_DIR)/rom-manifest-c$(EXEEXT): ../tools/rom_manifest.c ../common/sha256.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) "$<" -o "$@"
$(BUILD_DIR)/rom-manifest-cpp$(EXEEXT): ../tools/rom_manifest.cpp ../common/sha256.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-chr-c$(EXEEXT): ../graphics/nes_chr_viewer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-chr-cpp$(EXEEXT): ../graphics/nes_chr_viewer.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-apu-c$(EXEEXT): ../audio/nes_apu_log.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-apu-cpp$(EXEEXT): ../audio/nes_apu_log.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"

$(BUILD_DIR)/nes-dev-env: analysis/nes_dev_environment.py | $(BUILD_DIR)
	@printf '%s\n' '#!/bin/sh' 'exec python3 analysis/nes_dev_environment.py "$$@"' > "$@"
	chmod +x "$@"

# Build all six native terminal tools for the current operating system.
# CI publishes the matching OS-specific binaries as downloadable artifacts.
release-binaries: native
	@echo "Native C/C++ terminal binaries are ready in $(BUILD_DIR)/"
	@echo "Executables: nes-native-c$(EXEEXT), nes-native-cpp$(EXEEXT), nes-rom-build-c$(EXEEXT), nes-rom-build-cpp$(EXEEXT), nes-disasm-c$(EXEEXT), nes-disasm-cpp$(EXEEXT)"

test-python:
	python3 -m py_compile analysis/nes_inspect.py analysis/nes_disasm.py analysis/nes_workbench.py analysis/nes_dev_environment.py analysis/nes_circuit.py analysis/nes_advanced.py analysis/nes_rom_build.py examples/create_minimal_rom.py examples/inspect_existing.py
	python3 -m unittest analysis/test_nes_inspect.py analysis/test_nes_workbench.py analysis/test_nes_circuit.py analysis/test_nes_advanced.py analysis/test_nes_dev_environment.py analysis/test_nes_rom_build.py examples/test_examples.py -v
test-native: native
	@set -eu; mkdir -p "$(BUILD_DIR)/test-output"; python3 -c "from pathlib import Path; Path('$(BUILD_DIR)/synthetic.nes').write_bytes(b'NES\\x1a'+bytes([1,1])+bytes(10)+bytes(16384+8192))"; \
	"$(BUILD_DIR)/nes-native-c$(EXEEXT)" --out-dir "$(BUILD_DIR)/test-output/c" "$(BUILD_DIR)/synthetic.nes"; \
	"$(BUILD_DIR)/nes-native-cpp$(EXEEXT)" --out-dir "$(BUILD_DIR)/test-output/cpp" "$(BUILD_DIR)/synthetic.nes"; \
	"$(BUILD_DIR)/nes-disasm-c$(EXEEXT)" --out-dir "$(BUILD_DIR)/test-output/disasm-c" "$(BUILD_DIR)/synthetic.nes"; \
	"$(BUILD_DIR)/nes-disasm-cpp$(EXEEXT)" --out-dir "$(BUILD_DIR)/test-output/disasm-cpp" "$(BUILD_DIR)/synthetic.nes"; \
	"$(BUILD_DIR)/nes-rom-build-c$(EXEEXT)" --input "$(BUILD_DIR)/synthetic.nes" --output "$(BUILD_DIR)/test-output/c-patched.nes" --patch 16 00 01; \
	"$(BUILD_DIR)/nes-rom-build-cpp$(EXEEXT)" --input "$(BUILD_DIR)/synthetic.nes" --output "$(BUILD_DIR)/test-output/cpp-patched.nes" --patch 16 00 02; \
	test "$$(wc -c < "$(BUILD_DIR)/synthetic.nes")" = "$$(wc -c < "$(BUILD_DIR)/test-output/c-patched.nes")"; \
	test "$$(od -An -tx1 -j16 -N1 "$(BUILD_DIR)/test-output/c-patched.nes" | tr -d ' \\n')" = "01"; \
	test "$$(od -An -tx1 -j16 -N1 "$(BUILD_DIR)/test-output/cpp-patched.nes" | tr -d ' \\n')" = "02"; \
	test -s "$(BUILD_DIR)/test-output/c/synthetic.nes/summary.md"; test -s "$(BUILD_DIR)/test-output/cpp/synthetic/summary.md"; \
	test -s "$(BUILD_DIR)/test-output/disasm-c/synthetic-native/disassembly.asm"; test -s "$(BUILD_DIR)/test-output/disasm-cpp/synthetic-native/disassembly.asm"
test-cli: cli
	$(BUILD_DIR)/nes-dev-env --help >/dev/null
test-analysis-native: analysis-native
	python3 ../tests/test_native_tooling.py "$(BUILD_DIR)"
test: test-python test-native test-analysis-native test-cli
examples-help:
	@cat examples/README.md
example-rom:
	python3 examples/create_minimal_rom.py --output "$(if $(OUTPUT),$(OUTPUT),$(BUILD_DIR)/examples/minimal.nes)"
example-inspect:
	@test -n "$(ROM)" || (echo "Usage: make example-inspect ROM=/path/to/game.nes [TITLE='Game Title'] [OUT=./workspaces]"; exit 2)
	python3 examples/inspect_existing.py "$(ROM)" $(if $(TITLE),--title "$(TITLE)",) --output-root "$(if $(OUT),$(OUT),./workspaces)"
dev-env-help:
	$(BUILD_DIR)/nes-dev-env --help
rom-build-help:
	python3 analysis/nes_rom_build.py --help
rom-project:
	@test -n "$(ROM)" || (echo "Usage: make rom-project ROM=/path/to/game.nes PROJECT=./game-project [TITLE='Game Title']"; exit 2)
	python3 analysis/nes_rom_build.py init "$(ROM)" --project "$(if $(PROJECT),$(PROJECT),nes-rom-project)" $(if $(TITLE),--title "$(TITLE)",)
rom-build:
	@test -n "$(PROJECT)" || (echo "Usage: make rom-build PROJECT=./game-project/rom-project.json [OUTPUT=build/modified.nes]"; exit 2)
	python3 analysis/nes_rom_build.py build --project "$(PROJECT)" $(if $(OUTPUT),--output "$(OUTPUT)",)
dev-env:
	@test -n "$(ROM)" || (echo "Usage: make dev-env ROM=/path/to/game.nes [TITLE='Game Title'] [OUT=/path/to/output-root] [TRACE=/path/to/trace.json] [RULES=/path/to/game-rules.json]"; exit 2)
	python3 analysis/nes_dev_environment.py "$(ROM)" $(if $(TITLE),--title "$(TITLE)",) $(if $(OUT),--output-root "$(OUT)",) $(if $(TRACE),--trace "$(TRACE)",) $(if $(RULES),--rules "$(RULES)",)
clean:
	rm -rf "$(BUILD_DIR)" build-cmake rom-analysis-output .nes-analysis-stage-*
help:
	@echo 'make [all|native|analysis-native|c|cpp|cli|release-binaries|test|test-python|test-native|test-analysis-native|test-cli|examples-help|example-rom [OUTPUT=...]|example-inspect ROM=... [TITLE=...] [OUT=...]|dev-env ROM=... [TITLE=...] [OUT=...] [TRACE=...]|rom-project ROM=... [PROJECT=...] [TITLE=...]|rom-build PROJECT=... [OUTPUT=...]|clean]'
