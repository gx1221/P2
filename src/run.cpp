#include "cpu.h"
#include <iostream>
#include <fstream>
#include <iomanip>

int main() {
    CPU cpu;

    // Initialize all opcodes
    cpu.init_opcode_table();

    // Loading rom
    cpu.loadROM("01-basics.nes");

    // Open log file to write opcodes in
    std::ofstream logFile("cpu_traces.log");
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file!\n";
        return 1;
    }

    const int max_cycles = 5000;
    int steps = 0;

    while (steps < max_cycles) {
        uint16_t current_pc = cpu.get_PC();

        // output status of all registers and PC to log file for testing
        uint8_t next_byte = cpu.read(current_pc + 1);
        logFile << "PC=$" << std::hex << std::setfill('0') << std::setw(4) << current_pc
            << "  OP=$" << std::setw(2) << (int)cpu.getCurrentOpcode()
            << "  NEXT=$" << std::setw(2) << (int)next_byte
            << "  X=$" << std::setw(2) << (int)cpu.get_X()
            << "  Y=$" << std::setw(2) << (int)cpu.get_Y()
            << "  A=$" << std::setw(2) << (int)cpu.get_A()
            << "  P=$" << std::setw(2) << (int)cpu.get_P()
            << "  SP=$" << std::setw(2) << (int)cpu.get_SP()
            << "  CYC=" << std::dec << cpu.get_cycles()
            << "\n";


        cpu.step();
        steps++;
    }

    logFile.close();
    std::cout << "Program stopped.\n";
    return 0;
}