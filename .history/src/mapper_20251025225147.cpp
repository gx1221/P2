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






    void Mapper1::update_banks() {
        uint8_t prg_mode = (control >> 2) & 0x03;
        size_t num_banks = prgROM.size() / 0x4000;
        if (!num_banks) {
            prg_bank_low = prg_bank_high = 0; 
            return;
        }

        switch (prg_mode) {
            case 0: 
            case 1: {
                uint8_t even = (uint8_t)((prg_bank & 0xFE) % num_banks);
                prg_bank_low  = even;
                prg_bank_high = (uint8_t)((even + 1) % num_banks);
            } break;
            case 2:
                prg_bank_low  = 0;
                prg_bank_high = (uint8_t)(prg_bank % num_banks);
                break;
            case 3:
                prg_bank_low  = (uint8_t)(prg_bank % num_banks);
                prg_bank_high = (uint8_t)(num_banks - 1);
                break;
        }
    }

    // returns offset into nametables vector (0..0x7FF)
    uint16_t Mapper1::mirrorAddress(uint16_t addr) {
        uint16_t a = addr & 0x0FFF;
        uint16_t nametable_index = (a / 0x400) & 0x03;
        uint16_t index = a % 0x400; 
        uint8_t mode = control & 0x03; 
        switch (mode) {
            case 0: // lower bank
                return index;
            case 1: // higher bank
                return 0x400 + index;      
            case 2: // vertical: NT0, NT1, NT0, NT1
                if (nametable_index == 0 || nametable_index == 2) return index;
                else return 0x400 + index;
            case 3: // horizontal: NT0, NT0, NT1 , NT1
                if (nametable_index == 0 || nametable_index == 1) return index;
                else return 0x400 + index;
        }
        return index;
    }


    Mapper1::Mapper1(std::vector<uint8_t> prg, std::vector<uint8_t> chr, bool vertical)
        : prgROM(prg), chrROM(chr), vertical_mirror(vertical) {
        nametables.resize(0x800, 0);
        palette.resize(32, 0);
        prgRAM.resize(0x2000, 0); 
        shift_reg = 0x10;
        write_count = 0;
        control = 0x0C;
        prg_ram_enable = true;
        prg_bank_high = (prgROM.size() / 0x4000) - 1;
        if (chrROM.empty()) {
          chrRAM.resize(0x2000, 0);
        }
        
        update_banks();
    }

    uint8_t Mapper1::read_cpu(uint16_t addr) {
        if (addr >= 0x6000 && addr < 0x8000) {
            printf("[PRG-RAM R] %04X -> %02X\n", addr, prgRAM[addr-0x6000]);
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


    void Mapper1::write_cpu(uint16_t addr, uint8_t data) {
        
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

uint8_t Mapper1::read_ppu(uint16_t addr) {
    addr &= 0x3FFF; // Wrap PPU address space

    if (addr < 0x2000) {
        uint8_t chr_mode = (control >> 4) & 1;

        if (chrROM.empty()) {
            // CHR RAM fallback
            return chrRAM[addr & 0x1FFF];
        } else {
            if (chr_mode == 0) {
                // 8 KB mode
                uint32_t bank = (chr_bank0 & 0x1E); // Ignore bit 0
                uint32_t bank_offset = bank * 0x1000; // 8 KB block (2 * 4 KB)
                uint32_t index = (bank_offset + (addr & 0x1FFF)) % chrROM.size();
                return chrROM[index];
            } else {
                // 4 KB mode
                if (addr < 0x1000) {
                    uint32_t bank0 = chr_bank0 & 0x1F;
                    uint32_t bank_offset0 = bank0 * 0x1000;
                    uint32_t index0 = (bank_offset0 + (addr & 0x0FFF)) % chrROM.size();
                    return chrROM[index0];
                } else {
                    uint32_t bank1 = chr_bank1 & 0x1F;
                    uint32_t bank_offset1 = bank1 * 0x1000;
                    uint32_t index1 = (bank_offset1 + (addr & 0x0FFF)) % chrROM.size();
                    return chrROM[index1];
                }
            }
        }
    } else if (addr < 0x3F00) {
        return nametables[mirrorAddress(addr)];
    } else {
        uint16_t index = addr & 0x1F;
        if ((index & 0x03) == 0) index &= 0x0F; // Mirror universal background
        return palette[index];
    }
}



    void Mapper1::write_ppu(uint16_t addr, uint8_t data) {
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


