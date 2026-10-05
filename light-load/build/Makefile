# Build driver — orchestrates our own tools into a bootable .nes.
CXX     ?= c++
CXXFLAGS?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

build_driver: build_driver.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f build_driver

.PHONY: clean
