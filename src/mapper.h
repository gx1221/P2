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

// Mapper0 / NROM subclass
class Mapper0 : public Mapper {
    std::vector<uint8_t> prgROM;
    std::vector<uint8_t> chrROM;
    std::vector<uint8_t> nametables;
    bool vertical_mirror;
    uint16_t mirrorAddress(uint16_t addr, bool verticalMirror);

public:
    Mapper0(std::vector<uint8_t> prg, std::vector<uint8_t> chr, bool vertical);
    uint8_t read_cpu(uint16_t addr) override;
    void write_cpu(uint16_t addr, uint8_t data) override;
    uint8_t read_ppu(uint16_t addr) override;
    void write_ppu(uint16_t addr, uint8_t data) override;
};

class Mapper1 : public Mapper {
    std::vector<uint8_t> prgROM;
    std::vector<uint8_t> chrROM;
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

    void update_banks() {
    uint8_t prg_mode = (control >> 2) & 0x03;
    //uint8_t chr_mode = (control >> 4) & 0x01;
    size_t num_banks = prgROM.size() / 0x4000;

    // PRG banking
    switch (prg_mode) {
        case 0: // 32 KB mode
        case 1:
            prg_bank_low = (prg_bank & 0xFE) % num_banks; // <-- FIXED
            prg_bank_high = prg_bank_low + 1;
            break;

        case 2: // fix first bank at $8000
            prg_bank_low = 0;
            prg_bank_high = prg_bank % num_banks; // <-- FIXED
            break;

        case 3: // switch $8000-$BFFF, fix last bank at $C000
            prg_bank_low = prg_bank % num_banks;   // <-- FIXED
            prg_bank_high = num_banks - 1;        // fixed last bank
            break;
    }

    // CHR banking is handled in read/write PPU, no changes here

   /* printf("[Mapper1] Control=%02X, PRG low=%d, PRG high=%d, CHR0=%d, CHR1=%d, PRG mode=%d, CHR mode=%d\n",
           control, prg_bank_low, prg_bank_high, chr_bank0, chr_bank1, prg_mode, chr_mode);*/
}


    uint16_t mirrorAddress(uint16_t addr) {
        uint8_t mode = control & 0x03;
        addr &= 0x0FFF;
        switch (mode) {
            case 0: return addr % 0x400;          // one-screen lower
            case 1: return 0x400 + (addr % 0x400);// one-screen upper
            case 2: return addr % 0x800;          // vertical
            case 3: return (addr < 0x800 || (addr >= 0x1000 && addr < 0x1800))
                          ? addr % 0x400 : 0x400 + (addr % 0x400); // horizontal
        }
        return addr % 0x800;
    }

public:
    Mapper1(std::vector<uint8_t> prg, std::vector<uint8_t> chr, bool vertical)
        : prgROM(prg), chrROM(chr), vertical_mirror(vertical) {
        nametables.resize(0x800, 0);
        prg_bank_high = (prgROM.size() / 0x4000) - 1; // high bank fixed
        shift_reg = 0x10;
        write_count = 0;
        control = 0x0C; // power-on PRG mode = 3
        update_banks();
    }

    uint8_t read_cpu(uint16_t addr) override {
        if (addr < 0x8000) return 0;

        uint32_t offset = 0;
        if (addr < 0xC000)
            offset = (addr - 0x8000) + (prg_bank_low * 0x4000);
        else
            offset = (addr - 0xC000) + (prg_bank_high * 0x4000);

        offset %= prgROM.size();
        return prgROM[offset];
    }


    void write_cpu(uint16_t addr, uint8_t data) override {
        
        if (addr < 0x8000) {
            return;
        }

        if (data & 0x80) {
            shift_reg = 0x10;
            write_count = 0;
            control |= 0x0C;
            update_banks();
            return;
        }

        shift_reg = (shift_reg >> 1) | ((data & 1) << 4);
        write_count++;

        if (write_count == 5) {
            uint8_t value = shift_reg & 0x1F;

            if (addr < 0xA000) {
                control = value;
            }
            else if (addr < 0xC000) {
                chr_bank0 = value;
            }
            else if (addr < 0xE000) {
                chr_bank1 = value;
            }
            else {
                prg_bank = value & 0x0F;
            }

            shift_reg = 0x10;
            write_count = 0;
            update_banks();
        }
    }

    uint8_t read_ppu(uint16_t addr) override {
        if (chrROM.empty()) {
            // CHR RAM fallback: read from nametables or just return 0
            return 0;
        }
        if (addr < 0x2000) {
            uint8_t chr_mode = (control >> 4) & 1;
            if (chr_mode == 0) {
                uint32_t bank_offset = (chr_bank0 & 0x1E) * 0x1000;
                return chrROM[(bank_offset + addr) % chrROM.size()];
            } else {
                if (addr < 0x1000)
                    return chrROM[((chr_bank0 * 0x1000) + addr) % chrROM.size()];
                else
                    return chrROM[((chr_bank1 * 0x1000) + (addr - 0x1000)) % chrROM.size()];
            }
        } else if (addr < 0x3F00) {
            return nametables[mirrorAddress(addr)];
        }
        return 0;
    }

    void write_ppu(uint16_t addr, uint8_t data) override {
        if (chrROM.empty()) {
            // CHR RAM fallback: read from nametables or just return 0
            return;
        }
        if (addr < 0x2000 && !chrROM.empty()) {
            uint8_t chr_mode = (control >> 4) & 1;
            if (chr_mode == 0) {
                uint32_t bank_offset = (chr_bank0 & 0x1E) * 0x1000;
                chrROM[(bank_offset + addr) % chrROM.size()] = data;
            } else {
                if (addr < 0x1000)
                    chrROM[((chr_bank0 * 0x1000) + addr) % chrROM.size()] = data;
                else
                    chrROM[((chr_bank1 * 0x1000) + (addr - 0x1000)) % chrROM.size()] = data;
            }
        } else if (addr < 0x3F00) {
            nametables[mirrorAddress(addr)] = data;
        }
    }
};
