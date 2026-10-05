# Nintendo Technical Series — structural reader + discriminator
# Builds the C core and C++ layer into a single CLI: nts-analyze
CC      ?= cc
CXX     ?= c++
CFLAGS  ?= -std=c11   -O2 -Wall -Wextra -Wpedantic
CXXFLAGS?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
BIN      = nts-analyze

C_DIR    = c-ines-reader
CPP_DIR  = cpp-discriminator
GEN_DIR  = c-canvas-gen
BIN2     = nts-canvas
OBJS = $(C_DIR)/ines.o $(CPP_DIR)/conditions.o $(CPP_DIR)/main.o
GEN_OBJS = $(GEN_DIR)/canvas.o $(GEN_DIR)/main.o

all: $(BIN) $(BIN2)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

$(BIN2): $(GEN_OBJS)
	$(CC) $(CFLAGS) -o $@ $(GEN_OBJS)

$(GEN_DIR)/canvas.o: $(GEN_DIR)/canvas.c $(GEN_DIR)/canvas.h
	$(CC) $(CFLAGS) -c $< -o $@

$(GEN_DIR)/main.o: $(GEN_DIR)/main.c $(GEN_DIR)/canvas.h
	$(CC) $(CFLAGS) -c $< -o $@

$(C_DIR)/ines.o: $(C_DIR)/ines.c $(C_DIR)/ines.h
	$(CC) $(CFLAGS) -c $< -o $@

$(CPP_DIR)/conditions.o: $(CPP_DIR)/conditions.cpp $(CPP_DIR)/discriminator.hpp $(C_DIR)/ines.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CPP_DIR)/main.o: $(CPP_DIR)/main.cpp $(CPP_DIR)/discriminator.hpp $(C_DIR)/ines.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(GEN_OBJS) $(BIN) $(BIN2)

.PHONY: all clean
