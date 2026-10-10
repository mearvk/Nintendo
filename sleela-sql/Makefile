# sleela-sql -- CSV-backed MySQL-subset engine (C build).
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L -Iinclude
BIN := sleela-sql
OBJ := src/sleela_sql.o src/sleela_sql_cli.o

all: $(BIN) libsleelasql.a

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

libsleelasql.a: src/sleela_sql.o
	ar rcs $@ $^

src/%.o: src/%.c include/sleela_sql.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(BIN) libsleelasql.a

.PHONY: all clean
