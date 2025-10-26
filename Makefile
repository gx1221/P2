# Compiler and flags
CXX      := g++
CXXFLAGS = -g -Werror -Wall -std=c++17 `sdl2-config --cflags`
#-g -Werror -Wall -Wextra -Wpedantic -std=c++17 `sdl2-config --cflags` -Wcast-align -Wcast-qual -Wfloat-equal -Wformat=2 -Wlogical-op -Wmissing-include-dirs -Wpointer-arith -Wredundant-decls -Wsequence-point -Wshadow -Wswitch -Wundef -Wunreachable-code -Wunused-but-set-parameter -Wwrite-strings

LDFLAGS = `sdl2-config --libs`
SRCS = src/cpu.cpp src/mapper.cpp src/ppu.cpp src/apu.cpp src/input.cpp

lazy: src/test.cpp
	$(CXX) $(CXXFLAGS) $< $(SRCS) -o test $(LDFLAGS) && ./test

debug: src/test.cpp
	$(CXX) $(CXXFLAGS) $< $(SRCS) -o debug $(LDFLAGS)
	@echo "Running under gdb..."
	@gdb ./debug

valgrind: src/test.cpp
	$(CXX) $(CXXFLAGS) $< $(SRCS) -o debug $(LDFLAGS)
	@echo "Running under valgrind..."
	@valgrind --leak-check=full --track-origins=yes ./debug