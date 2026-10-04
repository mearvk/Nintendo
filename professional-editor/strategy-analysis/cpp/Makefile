# NES Strategy Analysis -- C++ build.
CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude
BIN := nes-strat-cpp
OBJ := src/NesStrategy.o src/nes_strat_main.o

all: $(BIN) libnesstrat_cpp.a

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

libnesstrat_cpp.a: src/NesStrategy.o
	ar rcs $@ $^

src/%.o: src/%.cpp include/NesStrategy.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(BIN) libnesstrat_cpp.a

.PHONY: all clean
