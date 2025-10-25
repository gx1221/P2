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
    std::vector<uint8_t> palette;
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
    bool has_chr_ram = false;


    void update_banks() {
    uint8_t prg_mode = (control >> 2) & 0x03;
    //uint8_t chr_mode = (control >> 4) & 0x01;
    size_t num_banks = prgROM.size() / 0x4000;

    // PRG banking
    switch (prg_mode) {
        case 0: // 32 KB mode
        case 1:
            prg_bank_low = (prg_bank & 0xFE) % num_banks;
            prg_bank_high = prg_bank_low + 1;
            break;

        case 2: // fix first bank at $8000
            prg_bank_low = 0;
            prg_bank_high = prg_bank % num_banks;
            break;

        case 3: // switch $8000-$BFFF, fix last bank at $C000
            prg_bank_low = prg_bank % num_banks; 
            prg_bank_high = num_banks - 1;        
            break;
    }

    // debug statement. I'll leave it here

   /* printf("[Mapper1] Control=%02X, PRG low=%d, PRG high=%d, CHR0=%d, CHR1=%d, PRG mode=%d, CHR mode=%d\n",
           control, prg_bank_low, prg_bank_high, chr_bank0, chr_bank1, prg_mode, chr_mode);*/
}


    // returns offset into nametables vector (0..0x7FF)
    uint16_t mirrorAddress(uint16_t addr) {
        // normalize to 0..0xFFF for the 4 nametables (0x2000..0x2FFF -> 0..0xFFF)
        uint16_t a = addr & 0x0FFF;
        uint16_t nt_index = (a / 0x400) & 0x03; // which nametable 0..3
        uint16_t index = a % 0x400; // offset inside that nametable
        uint8_t mode = control & 0x03; // mirroring bits

        switch (mode) {
            case 0: // lower bank
                return index;
            case 1: // higher bank
                return 0x400 + index;      
            case 2: // vertical: NT0, NT1, NT0, NT1
                if (nt_index == 0 || nt_index == 2) return index;
                else return 0x400 + index;
            case 3: // horizontal: NT0, NT0, NT1 , NT1
                if (nt_index == 0 || nt_index == 1) return index;
                else return 0x400 + index;
        }
        return index;
    }


public:
    Mapper1(std::vector<uint8_t> prg, std::vector<uint8_t> chr, bool vertical)
        : prgROM(prg), chrROM(chr), vertical_mirror(vertical) {
        nametables.resize(0x800, 0);
        palette.resize(32, 0);
        prgRAM.resize(0x2000, 0); 
        shift_reg = 0x10;
        write_count = 0;
        control = 0x0C;
        prg_ram_enable = true;
        prg_bank_high = (prgROM.size() / 0x4000) - 1;
        has_chr_ram = chrROM.empty();              
        if (has_chr_ram) {
            chrRAM.resize(0x2000, 0);
        }
        
        update_banks();
    }

    uint8_t read_cpu(uint16_t addr) override {
        if (addr >= 0x6000 && addr < 0x8000) {
            return prgRAM[addr - 0x6000];
        }
        if (addr < 0x8000) {
            return 0xFF;
        }

        uint32_t offset = 0;
        if (addr < 0xC000)
            offset = (addr - 0x8000) + (prg_bank_low * 0x4000);
        else {
            offset = (addr - 0xC000) + (prg_bank_high * 0x4000);
        }

        if (prgROM.empty()) {
            return 0xFF;
        }

        offset %= prgROM.size();
        return prgROM[offset];
    }


    void write_cpu(uint16_t addr, uint8_t data) override {
        
        if (addr >= 0x6000 && addr < 0x8000) {
            if (prg_ram_enable) {
                prgRAM[addr - 0x6000] = data;
            }
            return;
        }

        if (addr < 0x8000) {
            return;
        }

        // reset MMC1
        if (data & 0x80) {
            shift_reg = 0x10;
            write_count = 0;
            control |= 0x0C;
            update_banks();
            return;
        }

        //write into bit 4
        shift_reg = (shift_reg >> 1) | ((data & 1) << 4);
        write_count++;

        if (write_count == 5) {
            uint8_t value = shift_reg & 0x1F;
            switch ((addr >> 13) & 0x03) {
                case 0: 
                    control = value;
                    break; 
                case 1: 
                    chr_bank0 = value;
                    break; 
                case 2: 
                    chr_bank1 = value;
                    break; 
                case 3:
                    prg_bank = (uint8_t)(value & 0x0F);
                    prg_ram_enable = ((value & 0x10) == 0);
                    break;
                }

            shift_reg = 0x10;
            write_count = 0;
            update_banks();
        }
    }

    uint8_t read_ppu(uint16_t addr) override {
        addr &= 0x3FFF; // PPU address space mirrors every 0x4000

        if (addr < 0x2000) {
            uint8_t chr_mode = (control >> 4) & 1;
            if (chrROM.empty()) {
                return chrRAM[addr & 0x1FFF];
            } else {
                if (chr_mode == 0) {
                    // 8 KB mode
                    uint32_t bank_index = (chr_bank0 & 0x1E);
                    uint32_t bank_offset = bank_index * 0x1000; // bank_index counts in 4KB units
                    uint32_t idx = (bank_offset + (addr & 0x1FFF)) % chrROM.size();
                    return chrROM[idx];
                } else {
                    // 4 KB mode
                    if (addr < 0x1000) {
                        uint32_t bank_offset = (uint32_t)(chr_bank0 & 0x1F) * 0x1000;
                        uint32_t idx = (bank_offset + (addr & 0x0FFF)) % chrROM.size();
                        return chrROM[idx];
                    } else {
                        uint32_t bank_offset = (uint32_t)(chr_bank1 & 0x1F) * 0x1000;
                        uint32_t idx = (bank_offset + (addr & 0x0FFF)) % chrROM.size();
                        return chrROM[idx];
                    }
                }
            }
        }
        else if (addr < 0x3F00) {
            return nametables[mirrorAddress(addr)];
        }
        else {
            uint16_t index = addr & 0x1F;         
            if ((index & 0x03) == 0) index &= 0x0F;
            return palette[index];
        }
    }



    void write_ppu(uint16_t addr, uint8_t data) override {
        addr &= 0x3FFF; // mirroring

        if (addr < 0x2000) {
            if (chrROM.empty()) {
                chrRAM[addr & 0x1FFF] = data;
            }
        }
        else if (addr < 0x3F00) {
            nametables[mirrorAddress(addr)] = data;
        }
        else {
            uint16_t index = addr & 0x1F;
            if ((index & 0x03) == 0) index &= 0x0F; 
            palette[index] = data;
        }
    }

};
