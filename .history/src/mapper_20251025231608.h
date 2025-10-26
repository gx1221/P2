#pragma once
#include <cstdlib>
#include <stdint.h>
#include <vector>
#include <cstdio>

class Mapper {
public:
    virtual uint8_t read_cpu(uint16_t addr) = 0;
    virtual void write_cpu(uint16_t addr, uint8_t data) = 0;
    virtual uint8_t read_ppu(uint16_t addr) = 0;
    virtual void write_ppu(uint16_t addr, uint8_t data) = 0;
    virtual ~Mapper() = default;
};

// Mapper0 / NROM
class Mapper0 : public Mapper {
    std::vector<uint8_t> prgROM; //prgROM is the actual program
    std::vector<uint8_t> chrROM; //chrROM is the sprites/characters
    std::vector<uint8_t> nametables; //Table to layout every tile and to map everything
    bool vertical_mirror;
    uint16_t mirrorAddress(uint16_t addr, bool verticalMirror); // finds mirrored nametable based on address

public:
    Mapper0(std::vector<uint8_t> prg, std::vector<uint8_t> chr, bool vertical);
    uint8_t read_cpu(uint16_t addr) override;
    void write_cpu(uint16_t addr, uint8_t data) override;
    uint8_t read_ppu(uint16_t addr) override;
    void write_ppu(uint16_t addr, uint8_t data) override;
};

class Mapper1 : public Mapper {
private:
    std::vector<uint8_t> prgROM;
    std::vector<uint8_t> chrROM;
    std::vector<uint8_t> palette; // sprite palette for color
    std::vector<uint8_t> prgRAM;
    std::vector<uint8_t> chrRAM; 
    std::vector<uint8_t> nametables;

    uint8_t shift_reg = 0x10;
    uint8_t write_count = 0;
    uint8_t control = 0x0C;
    uint8_t chr_bank0 = 0;
    uint8_t chr_bank1 = 0;
    uint8_t prg_bank = 0;
    
    uint8_t prg_bank_low = 0;
    uint8_t prg_bank_high = 0;

    bool vertical_mirror;
    bool prg_ram_enable;

    
    // Updates the prg_bank_low and prg_bank_high indices based on the control register.
    void update_banks();

    // finds mirrored nametable index (0x000 - 0x7FF) from a PPU address (0x2000 - 0x2FFF).
    uint16_t mirrorAddress(uint16_t addr);

public:

    // Constructor: Initializes the mapper with PRG/CHR data and the board's default mirroring.
    Mapper1(std::vector<uint8_t> prg, std::vector<uint8_t> chr, bool vertical);

    // Reads a byte from the CPU address space ($6000-$FFFF).
    uint8_t read_cpu(uint16_t addr) override;

    // Writes a byte to the CPU address space ($6000-$FFFF).
    // Writes in the $8000-$FFFF range control the MMC1 shift register.
    void write_cpu(uint16_t addr, uint8_t data) override;

    // Reads a byte from the PPU address space (VRAM, CHR, Palette).
    uint8_t read_ppu(uint16_t addr) override;

    // Writes a byte to the PPU address space (VRAM, CHR-RAM, Palette).
    void write_ppu(uint16_t addr, uint8_t data) override;
};