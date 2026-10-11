# Native and Python ROM analysis tools plus per-title workspace generator
CC ?= cc
CXX ?= c++
CFLAGS ?= -O2 -std=c99 -Wall -Wextra -Wpedantic
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
BUILD_DIR ?= build
.PHONY: all native c cpp cli test test-python test-native test-cli dev-env dev-env-help rom-build-help rom-project rom-build clean help
all: native cli
native: c cpp
c: $(BUILD_DIR)/nes-native-c $(BUILD_DIR)/nes-rom-build-c
cpp: $(BUILD_DIR)/nes-native-cpp $(BUILD_DIR)/nes-rom-build-cpp
cli: $(BUILD_DIR)/nes-dev-env
$(BUILD_DIR):
	mkdir -p "$@"
$(BUILD_DIR)/nes-native-c: analysis/nes_native.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-native-cpp: analysis/nes_native.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-rom-build-c: analysis/nes_rom_build.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-rom-build-cpp: analysis/nes_rom_build.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-dev-env: analysis/nes_dev_environment.py | $(BUILD_DIR)
	@printf '%s\\n' '#!/bin/sh' 'exec python3 analysis/nes_dev_environment.py "$$@"' > "$@"
	chmod +x "$@"
test-python:
	python3 -m py_compile analysis/nes_inspect.py analysis/nes_disasm.py analysis/nes_workbench.py analysis/nes_dev_environment.py analysis/nes_circuit.py analysis/nes_advanced.py analysis/nes_rom_build.py
	python3 -m unittest analysis/test_nes_inspect.py analysis/test_nes_workbench.py analysis/test_nes_circuit.py analysis/test_nes_advanced.py analysis/test_nes_dev_environment.py analysis/test_nes_rom_build.py -v
test-native: native
	@set -eu; mkdir -p "$(BUILD_DIR)/test-output"; python3 -c "from pathlib import Path; Path('$(BUILD_DIR)/synthetic.nes').write_bytes(b'NES\\x1a'+bytes([1,1])+bytes(10)+bytes(16384+8192))"; \
	"$(BUILD_DIR)/nes-native-c" --out-dir "$(BUILD_DIR)/test-output/c" "$(BUILD_DIR)/synthetic.nes"; \
	"$(BUILD_DIR)/nes-native-cpp" --out-dir "$(BUILD_DIR)/test-output/cpp" "$(BUILD_DIR)/synthetic.nes"; \
	"$(BUILD_DIR)/nes-rom-build-c" --input "$(BUILD_DIR)/synthetic.nes" --output "$(BUILD_DIR)/test-output/c-patched.nes" --patch 16 00 01; \
	"$(BUILD_DIR)/nes-rom-build-cpp" --input "$(BUILD_DIR)/synthetic.nes" --output "$(BUILD_DIR)/test-output/cpp-patched.nes" --patch 16 00 02; \
	test "$$(wc -c < "$(BUILD_DIR)/synthetic.nes")" = "$$(wc -c < "$(BUILD_DIR)/test-output/c-patched.nes")"; \
	test "$$(od -An -tx1 -j16 -N1 "$(BUILD_DIR)/test-output/c-patched.nes" | tr -d ' \\n')" = "01"; \
	test "$$(od -An -tx1 -j16 -N1 "$(BUILD_DIR)/test-output/cpp-patched.nes" | tr -d ' \\n')" = "02"; \
	test -s "$(BUILD_DIR)/test-output/c/synthetic.nes/summary.md"; test -s "$(BUILD_DIR)/test-output/cpp/synthetic/summary.md"
test-cli: cli
	$(BUILD_DIR)/nes-dev-env --help >/dev/null
test: test-python test-native test-cli
dev-env-help:
	build/nes-dev-env --help
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
	@echo 'make [all|native|c|cpp|cli|test|test-python|test-native|test-cli|dev-env-help|dev-env ROM=... [TITLE=...] [OUT=...] [TRACE=...]|rom-project ROM=... [PROJECT=...] [TITLE=...]|rom-build PROJECT=... [OUTPUT=...]|clean]'
