#include "cpu.h"
#include "ppu.h"

int main() {
    PPU ppu_placeholder(*(CPU*)nullptr); // temporary null CPU for init
    CPU cpu(ppu_placeholder);
    PPU ppu(cpu); // reassign CPU reference
    cpu.loadROM("nestest.nes");

    cpu.init_opcode_table(); // fill opcodes (or at least illegal instruction)

    while (true) {
        cpu.step();     // execute 1 CPU instruction

        // Tick the PPU for enough cycles per CPU step
        for (int i = 0; i < 3; ++i) // NES PPU runs 3x CPU cycles
            ppu.tick();

        // Check for NMI
        if (ppu.getNMI()) {
            ppu.setNMI(false);
            cpu.nmi();
        }
    }

    return 0;
}
