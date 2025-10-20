#include "mapper.h"

Mapper0::Mapper0(std::vector<uint8_t> prg, std::vector<uint8_t> chr, bool vertical)
    : prgROM(prg), chrROM(chr), vertical_mirror(vertical) 
{
    // allocate 2KB for nametables (4 nametables * 1KB each mirrored)
    nametables.resize(0x800, 0);
    if (chrROM.empty()) {
        chrROM.resize(0x2000, 0);
    }
}

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
    if (addr < 0x2000) {
        // CHR ROM/RAM
        return chrROM[addr];
    } else if (addr >= 0x2000 && addr < 0x3000) {
        // nametable reads with mirroring
        uint16_t mirrored = mirrorAddress(addr, vertical_mirror);
        return nametables[mirrored];
    } else if (addr >= 0x3000 && addr < 0x3F00) {
        // mirrors of nametables
        uint16_t mirrored = mirrorAddress(0x2000 + ((addr - 0x3000) % 0x1000), vertical_mirror);
        return nametables[mirrored];
    }
    return 0;
}

void Mapper0::write_ppu(uint16_t addr, uint8_t data) {
    if (addr < 0x2000 && !chrROM.empty()) {
        // CHR-RAM
        chrROM[addr] = data;
    } else if (addr >= 0x2000 && addr < 0x3000) {
        // nametable write with mirroring
        uint16_t mirrored = mirrorAddress(addr, vertical_mirror);
        nametables[mirrored] = data;
    } else if (addr >= 0x3000 && addr < 0x3F00) {
        // mirrored nametable write
        uint16_t mirrored = mirrorAddress(0x2000 + ((addr - 0x3000) % 0x1000), vertical_mirror);
        nametables[mirrored] = data;
    }
}

uint16_t Mapper0::mirrorAddress(uint16_t addr, bool verticalMirror) {
    addr = addr & 0x0FFF; // wrap to $2000-$2FFF
    if (verticalMirror) { 
        return addr % 0x800; // vertical mirroring
    } else {
        if (addr < 0x0800 || (addr >= 0x1000 && addr < 0x1800))
            return addr % 0x400;
        else
            return 0x400 + (addr % 0x400);
    }
}

