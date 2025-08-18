#include "cpu.h"
#include <iostream>

int main() {
    CPU cpu;

    // Initialize all opcodes
    cpu.init_opcode_table();

    // Load your test ROM
    cpu.loadROM("nestest.nes");  // replace with your ROM filename

    const int max_cycles = 100000; // run at most 100k steps
    int steps = 0;

    while (steps < max_cycles) {
        cpu.step();
        steps++;
    }

    std::cout << "program stopped";
    return 0;
}
