#include "cpu.h"

uint8_t CPU::read(uint16_t addr) const {
  if (addr >= 0x2000 && addr < 0x4000) {
    //call ppu function
  }
  return CPU_memory[addr]; 
}

void CPU::write(uint16_t addr, uint8_t value) { 
  if (addr >= 0x2000 && addr < 0x4000) {
      //call ppu function
  }
  CPU_memory[addr] = value; 
}