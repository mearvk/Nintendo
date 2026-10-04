# NES Professional Editor -- C build.
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Iinclude
BIN := nes-edit
OBJ := src/nes_rom.o src/nes_edit_cli.o

all: $(BIN) libnesrom.a

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

libnesrom.a: src/nes_rom.o
	ar rcs $@ $^

src/%.o: src/%.c include/nes_rom.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(BIN) libnesrom.a

.PHONY: all clean
