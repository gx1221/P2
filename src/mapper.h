#pragma once
#include <cstdlib>
#include <stdint.h>
#include <vector>


class Mapper { //creating Mapper class to make subclasses for scalability
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
    bool vertical_mirror;

public:
    Mapper0(std::vector<uint8_t> prg, std::vector<uint8_t> chr, bool vertical);
    uint8_t read_cpu(uint16_t addr) override;
    void write_cpu(uint16_t addr, uint8_t data) override;
    uint8_t read_ppu(uint16_t addr) override;
    void write_ppu(uint16_t addr, uint8_t data) override;
};
