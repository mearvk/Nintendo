# NES Professional Editor -- C++ build.
CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude
BIN := nes-edit-cpp
OBJ := src/NesRom.o src/nes_edit_main.o

all: $(BIN) libnesrom_cpp.a

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

libnesrom_cpp.a: src/NesRom.o
	ar rcs $@ $^

src/%.o: src/%.cpp include/NesRom.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(BIN) libnesrom_cpp.a

.PHONY: all clean
