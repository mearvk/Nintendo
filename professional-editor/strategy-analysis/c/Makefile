# NES Strategy Analysis -- C build.
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Iinclude
BIN := nes-strat
OBJ := src/nes_static.o src/nes_evidence.o src/nes_strat_cli.o

all: $(BIN) libnesstrat.a

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

libnesstrat.a: src/nes_static.o src/nes_evidence.o
	ar rcs $@ $^

src/%.o: src/%.c include/nes_strategy.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(BIN) libnesstrat.a

.PHONY: all clean
