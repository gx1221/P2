# Compiler and flags
CXX      := g++
CXXFLAGS = #-g -Werror -Wall -Wextra -Wpedantic -std=c++17 `sdl2-config --cflags` -Wcast-align -Wcast-qual -Wfloat-equal -Wformat=2 -Wlogical-op -Wmissing-include-dirs -Wpointer-arith -Wredundant-decls -Wsequence-point -Wshadow -Wswitch -Wundef -Wunreachable-code -Wunused-but-set-parameter -Wwrite-strings

LDFLAGS = `sdl2-config --libs`
SRCS = src/display.cpp src/cpu.cpp src/mapper.cpp src/ppu.cpp src/apu.cpp

lazy: src/test.cpp
	$(CXX) $(CXXFLAGS) $< $(SRCS) -o test $(LDFLAGS) && ./test

lazy2: src/test2.cpp
	$(CXX) $(CXXFLAGS) $< $(SRCS) -o test $(LDFLAGS) && ./test

run: src/run.cpp
		$(CXX) $(CXXFLAGS) $< $(SRCS) -o run $(LDFLAGS) && ./run

