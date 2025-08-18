#include "mapper.h"


Mapper0::Mapper0(std::vector<uint8_t> prg, std::vector<uint8_t> chr, bool vertical)
    : prgROM(prg), chrROM(chr), vertical_mirror(vertical) {}

uint8_t Mapper0::read_cpu(uint16_t addr) {
    if (addr >= 0x8000) {
        if (prgROM.size() == 0x4000) 
            return prgROM[(addr - 0x8000) % 0x4000];
        else
            return prgROM[addr - 0x8000];
    }
    return 0;
}

void Mapper0::write_cpu(uint16_t addr, uint8_t data) {
    (void)addr;
    (void)data;
}

uint8_t Mapper0::read_ppu(uint16_t addr) {
    if (addr < 0x2000) return chrROM[addr];
    return 0;
}

void Mapper0::write_ppu(uint16_t addr, uint8_t data) {
    if (addr < 0x2000 && !chrROM.empty()) chrROM[addr] = data;
}



uint16_t mirrorAddress(uint16_t addr, bool verticalMirror) {
    addr = addr & 0x0FFF; // wrap to $2000-$2FFF
    if (verticalMirror) { 
      return addr % 0x800; // vertical mirroring
    } 
    else {
      if (addr < 0x0800 || (addr >= 0x1000 && addr < 0x1800))
        return addr % 0x400;
      else
        return 0x400 + (addr % 0x400);
    }
}

