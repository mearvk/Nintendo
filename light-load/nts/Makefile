# Nintendo Technical Series — structural reader + discriminator
# Builds the C core and C++ layer into a single CLI: nts-analyze
CC      ?= cc
CXX     ?= c++
CFLAGS  ?= -std=c11   -O2 -Wall -Wextra -Wpedantic
CXXFLAGS?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
BIN      = nts-analyze

OBJS = c/ines.o cpp/conditions.o cpp/main.o

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

c/ines.o: c/ines.c c/ines.h
	$(CC) $(CFLAGS) -c $< -o $@

cpp/conditions.o: cpp/conditions.cpp cpp/discriminator.hpp c/ines.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

cpp/main.o: cpp/main.cpp cpp/discriminator.hpp c/ines.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all clean
