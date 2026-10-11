# Native and Python ROM analysis tools plus per-title workspace generator
CC ?= cc
CXX ?= c++
CFLAGS ?= -O2 -std=c99 -Wall -Wextra -Wpedantic
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
BUILD_DIR ?= build
.PHONY: all native c cpp cli test test-python test-native test-cli dev-env dev-env-help clean help
all: native cli
native: c cpp
c: $(BUILD_DIR)/nes-native-c
cpp: $(BUILD_DIR)/nes-native-cpp
cli: $(BUILD_DIR)/nes-dev-env
$(BUILD_DIR):
	mkdir -p "$@"
$(BUILD_DIR)/nes-native-c: analysis/nes_native.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-native-cpp: analysis/nes_native.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) "$<" -o "$@"
$(BUILD_DIR)/nes-dev-env: analysis/nes_dev_environment.py | $(BUILD_DIR)
	@printf '%s\n' '#!/bin/sh' 'exec python3 analysis/nes_dev_environment.py "$$@"' > "$@"
	chmod +x "$@"
test-python:
	python3 -m py_compile analysis/nes_inspect.py analysis/nes_disasm.py analysis/nes_workbench.py analysis/nes_dev_environment.py analysis/nes_circuit.py analysis/nes_advanced.py
	python3 -m unittest analysis/test_nes_inspect.py analysis/test_nes_workbench.py analysis/test_nes_circuit.py analysis/test_nes_advanced.py analysis/test_nes_dev_environment.py -v
test-native: native
	@set -eu; mkdir -p "$(BUILD_DIR)/test-output"; python3 -c "from pathlib import Path; Path('$(BUILD_DIR)/synthetic.nes').write_bytes(b'NES\\x1a'+bytes([1,1])+bytes(10)+bytes(16384+8192))"; "$(BUILD_DIR)/nes-native-c" --out-dir "$(BUILD_DIR)/test-output/c" "$(BUILD_DIR)/synthetic.nes"; "$(BUILD_DIR)/nes-native-cpp" --out-dir "$(BUILD_DIR)/test-output/cpp" "$(BUILD_DIR)/synthetic.nes"; test -s "$(BUILD_DIR)/test-output/c/synthetic.nes/summary.md"; test -s "$(BUILD_DIR)/test-output/cpp/synthetic/summary.md"
test-cli: cli
	$(BUILD_DIR)/nes-dev-env --help >/dev/null
test: test-python test-native test-cli
dev-env-help:
	build/nes-dev-env --help
dev-env:
	@test -n "$(ROM)" || (echo "Usage: make dev-env ROM=/path/to/game.nes [TITLE='Game Title'] [OUT=/path/to/output-root] [TRACE=/path/to/trace.json] [RULES=/path/to/game-rules.json]"; exit 2)
	python3 analysis/nes_dev_environment.py "$(ROM)" $(if $(TITLE),--title "$(TITLE)",) $(if $(OUT),--output-root "$(OUT)",) $(if $(TRACE),--trace "$(TRACE)",) $(if $(RULES),--rules "$(RULES)",)
clean:
	rm -rf "$(BUILD_DIR)" build-cmake rom-analysis-output .nes-analysis-stage-*
help:
	@echo 'make [all|native|c|cpp|cli|test|test-python|test-native|test-cli|dev-env-help|dev-env ROM=... [TITLE=...] [OUT=...] [TRACE=...]|clean]'
