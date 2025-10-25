#pragma once
#include <assert.h>
#include <cstdlib>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>

#include "mapper.h"

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_UNUSED    0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_NEGATIVE  0x80


class PPU; // forward declaration
class Input;
class CPU {
  public:
    PPU* ppu;
    Input* input;
    Mapper* mapper; 

    CPU();
    void connectPPU(PPU*);
    void connectInput(Input*);
    typedef void (*instruction_table)(void);

    //look up table to store all functions pointers
    void (CPU::*opcode_table[256])();

    uint8_t get_A();
    uint8_t get_X();
    uint8_t get_Y();
    uint8_t get_SP();
    uint8_t get_P();
    uint16_t get_PC();
    uint64_t& get_cycles();
    Mapper& get_mapper();
    uint8_t getCurrentOpcode() const;


    uint8_t print_opcode();
    void set_flag(uint8_t, bool);
    uint8_t read(uint16_t) const;
		void write(uint16_t, uint8_t);
    void push(uint8_t);
    uint8_t pop();
    void loadROM(const std::string&);
    uint8_t fetch();
    void step(); // Can't call it cycle since some instructions use multiple cycles

    void nmi();
    void init_opcode_table();

    void illegal_instruction();
    void adc_immediate();
    void adc_zeropage();
    void adc_zeropage_x();
    void adc_absolute();
    void adc_absolute_x();
    void adc_absolute_y();
    void adc_indexed_indirect();
    void adc_indirect_indexed();

    void and_immediate();
    void and_zeropage();
    void and_zeropage_x();
    void and_absolute();
    void and_absolute_x();
    void and_absolute_y();
    void and_indexed_indirect();
    void and_indirect_indexed();

    void asl_accumulator();
    void asl_zeropage();
    void asl_zeropage_x();
    void asl_absolute();
    void asl_absolute_x();

    void bcc_relative();

    void bcs_relative();

    void beq_relative();

    void bit_zeropage();

    void bit_absolute();

    void bmi_relative();
    
    void bne_relative();
    
    void bpl_relative();
    
    void brk_immediate();

    void brk_implied();
    
    void bvc_relative();

    void bvs_relative();
    

    void clc_implied();

    void cld_implied();
    
    void cli_implied();
    
    void clv_implied();

    void cmp_immediate();
    void cmp_zeropage();
    void cmp_zeropage_x();
    void cmp_absolute();
    void cmp_absolute_x();
    void cmp_absolute_y();
    void cmp_indexed_indirect();
    void cmp_indirect_indexed();

    void cpx_immediate();
    void cpx_zeropage();
    void cpx_absolute();

    void cpy_immediate();
    void cpy_zeropage();
    void cpy_absolute();

    void dec_zeropage();
    void dec_zeropage_x();
    void dec_absolute();
    void dec_absolute_x();

    void dex_implied();
    void dey_implied();

    void eor_immediate();
    void eor_zeropage();
    void eor_zeropage_x();
    void eor_absolute();
    void eor_absolute_x();
    void eor_absolute_y();
    void eor_indexed_indirect();
    void eor_indirect_indexed();

    void inc_zeropage();
    void inc_zeropage_x();
    void inc_absolute();
    void inc_absolute_x();

    void inx_implied();
    void iny_implied();

    void jmp_absolute();
    void jmp_indirect();

    void jsr_absolute();

    void lda_immediate();
    void lda_zeropage();
    void lda_zeropage_x();
    void lda_zeropage_y();
    void lda_absolute();
    void lda_absolute_x();
    void lda_absolute_y();
    void lda_indexed_indirect();
    void lda_indirect_indexed();

    void ldx_immediate();
    void ldx_zeropage();
    void ldx_zeropage_y();
    void ldx_absolute();
    void ldx_absolute_y();

    void ldy_immediate();
    void ldy_zeropage();
    void ldy_zeropage_x();
    void ldy_absolute();
    void ldy_absolute_x();

    void lsr_accumulator();
    void lsr_zeropage();
    void lsr_zeropage_x();
    void lsr_absolute();
    void lsr_absolute_x();

    void nop_implied();

    void ora_immediate();
    void ora_zeropage();
    void ora_zeropage_x();
    void ora_absolute();
    void ora_absolute_x();
    void ora_absolute_y();
    void ora_indexed_indirect();
    void ora_indirect_indexed();

    void pha_implied();
    void php_implied();
    void pla_implied();
    void plp_implied();

    void rol_accumulator();
    void rol_zeropage();
    void rol_zeropage_x();
    void rol_absolute();
    void rol_absolute_x();

    void ror_accumulator();
    void ror_zeropage();
    void ror_zeropage_x();
    void ror_absolute();
    void ror_absolute_x();

    void rti_implied();
    void rts_implied();

    void sbc_immediate();
    void sbc_zeropage();
    void sbc_zeropage_x();
    void sbc_absolute();
    void sbc_absolute_x();
    void sbc_absolute_y();
    void sbc_indexed_indirect();
    void sbc_indirect_indexed();

    void sec_implied();
    void sed_implied();
    void sei_implied();

    void sta_zeropage();
    void sta_zeropage_x();
    void sta_absolute();
    void sta_absolute_x();
    void sta_absolute_y();
    void sta_indexed_indirect();
    void sta_indirect_indexed();

    void stx_zeropage();
    void stx_zeropage_y();
    void stx_absolute();

    void sty_zeropage();
    void sty_zeropage_x();
    void sty_absolute();

    void tax_implied();
    void tay_implied();
    void tsx_implied();
    void txa_implied();
    void txs_implied();
    void tya_implied();

    
  
  private:
    // Accumulator. supports using status register for carrying and overflow detection
    uint8_t A;

    // Program counter
    // PC is the current memory location. It always points to the next instruction to be
    // executed, so when reading the PC, it will actually just return the next memory location
    // 16 bits because memory locations can go up to 65,536 bytes or 0xFFFF instead of only 256
    // remember, the PC literally just stores a value, it's not an array where when incrementing
    // makes it goes up by two positions in an array, when you increment this, it just adds 1 to the value
    // it doesn't reference anything at all.
    uint16_t PC; 

    // X and Y indexes. Loop counters
    uint8_t X;
    uint8_t Y;

    /*
      Stack pointer. The stack itself
      is located inside the memory/RAM in the first page (0x0100–0x01FF),
      In our CPU, our stack grows down, so we start off from
      the highest memory location of the stack and then go down

      Stack pointer points into page 1: 0x0100–0x01FF 256 bytes

      SP itself is just an 8-bit offset from 0x0100 
      so when pushing to the stack 
      the real location to push would be memory[0x100 + SP]

      the stack pointer points to the first "Available" memory location
      then goes down to the next available spot to show that it's
      available. For example, If SP = 0xFF, and we push, 0x01FF
      is filled, then SP-- to 0xFE.

      initialize to 0xFF after reset to start at beginning 0x01FF
    */
    uint8_t SP; 

    /*
      Status register. An 8-bit register. Each bit is a flag
      But only 6 of the bits actually are used by the cpu and arithmetic units
      7654 3210
      NV1B DIZC (128, 64, 32, 16, 8, 4, 2, 1 in binary) remember to OR
      Also multiple flags can be active at once
      The B flag and 1 do nothing

      #define FLAG_CARRY     0x01
      #define FLAG_ZERO      0x02
      #define FLAG_INTERRUPT 0x04
      #define FLAG_DECIMAL   0x08
      #define FLAG_BREAK     0x10
      #define FLAG_UNUSED    0x20
      #define FLAG_OVERFLOW  0x40
      #define FLAG_NEGATIVE  0x80

     */
    uint8_t P;

    // The total memory for the cpu is 64K bytes
    // Only the first 2k Bytes is ram Memory, but the rest
    // are for other functionality. 0x2000 - 0x3FFF are separate memory
    // locations for the ppu and require calling another function.
    uint8_t system_memory[65536];

    uint16_t reset_vector;
    
    bool bad_instruction;

    uint64_t cycles;

    uint8_t currentOpcode;  // store last fetched opcode
};

/*
===================================
 NES CPU Memory Map (Summary)
===================================

$0000–$07FF : 2KB internal RAM (2048 bytes)
$0800–$0FFF : Mirror of $0000–$07FF  
$1000–$17FF : Mirror of $0000–$07FF  
$1800–$1FFF : Mirror of $0000–$07FF  

$2000–$2007 : PPU registers  
$2008–$3FFF : Mirrors of $2000–$2007 (every 8 bytes)  

$4000–$4017 : APU and I/O registers  
$4018–$401F : Normally disabled APU and I/O functions (used in test mode)  

$4020–$5FFF : Open bus / cartridge expansion  

$6000–$7FFF : Cartridge RAM (if present)  
$8000–$FFFF : Cartridge ROM / Mapper registers  

$FFFA NMI Vector
$FFFC Reset Vector
$FFFE IRQ/BRK Vector

*/

/* .INES file format

1.  Header (16 bytes)
2.  Trainer, if present (0 or 512 bytes)
3.  PRG ROM data (16384 * x bytes) (x is flag 4)
4.  CHR ROM data, if present (8192 * y bytes)  (y is flag 5)
5.  PlayChoice INST-ROM, if present (0 or 8192 bytes)
6.  PlayChoice PROM, if present (This is often missing) 

Header format
___________________________________________________________________
bytes  | description
0-3 	 | Constant $4E $45 $53 $1A (ASCII "NES" followed by MS-DOS end-of-file)
4 	   | Size of PRG ROM in 16 KB units
5 	   | Size of CHR ROM in 8 KB units (value 0 means the board uses CHR RAM)
6 	   | Flags 6 – Mapper, mirroring, battery, trainer
7 	   | Flags 7 – Mapper, VS/Playchoice, NES 2.0
8 	   | Flags 8 – PRG-RAM size (rarely used extension)
9 	   | Flags 9 – TV system (rarely used extension)
10 	   | Flags 10 – TV system, PRG-RAM presence (unofficial, rarely used extension)
11-15  | Unused padding (should be filled with zero, but some rippers put their name across bytes 7-15) 
___________________________________________________________________
*/


/*
========================
 Opcode addressing modes
========================
All the examples are going to use LDX: load byte into X register
Also remember NES is little Endian, so after reading the opcode, if there's two
or more bytes, read the value backwards.

Immediate: literally just use the direct 8-bit (1 byte) value that comes after the opcode
Example: LDX #$FF -> X now contains FF
  uint8_t value = read(PC);
  PC++;


Zero Page: Take the 8-bit (1 byte) value, and use it as an index in the memory, then take the value 
that came from the memory location for use
Example: LDX #$FF -> read memory at 0x00FF and store value read into X
val = PEEK((arg % 256); you can also do (& 0xFF) instead 

If the value/variables are 8-bits, then you don't need to zero page it
It automatically zero pages it.
Example: 
  uint8_t address = read(PC);
  PC++;
  
  uint8_t value = read(address);

  
Zero Page X: Get the next 8 bit value and add it to whatever is in the X register.
After that, modulo the value you got by 0xFF to get the zero page and read the
value at the modulo'd memory location to get what you need
Example: LDA $FC -> if X = 0x04, then ((0xFC + 0x04) % 0xFF) = 0x01. then read(0x01) and store into A
val = PEEK((arg + X) % 256) 

remember, no need for the zero paging if variables are 8-bits

Example 2: 
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; 
  //add x to address then zero page
  uint8_t value = read(address);

Zero Page Y: same as zero page X, but with Y



Absolute: Get the next 16-bit (2 bytes) value and use it as an index.
(It's the zero page but with 16 bit memory location instead)
Example: LDX $FE $10 -> reads 0x10FE and stores read value in X (remember NES is little endian)
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  // 00hi -> hi00 -> hilo puts first byte into beginning and adds low byte using or since it takes over last 8 bits
  uint8_t value = read(address);


Absolute Indexed X: Take what's already in the X register add it to the value, then
use that sum as the index to read memory and take the value that to use

Example: LDA $33 $22 -> if X = 0x14, then you read(0x2233 + 0x14) and store it into A
val = PEEK(arg + X)	

Example 2:
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross
  if ((address & 0xFF00) != ((address + X) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }
  address += X;
  uint8_t value = read(address);  //read address with X added
  

Absolute Indexed Y: same as previous, but with Y register





Indexed indirect (d, X): you take the 8-bit value, add what's in the X register, then wrap it in the zero-page.
After that, you use this zero-paged address and the address+1 (not PC + 1, since PC can jump to other place) 
to get two bytes of values in the zero page, the low byte and high byte.
You take the bytes, combine them into a 16-bit value, and use that as an index to get your true value.
Example: LDA $20 ->
         Low = read((0x20 + X) & 0xFF)
         High = read((0x20 + X + 1) & 0xFF) # take the next byte after val + X
         address = high << 8 | low
         read(address)

Example 2:
  uint8_t initial_val = read(PC); 
  PC++;

  uint8_t low = read((initial_val + X) & 0xFF);
  uint8_t high = read((initial_val + X + 1) & 0xFF);

  uint16_t address = high << 8 | low;

  uint8_t value = read(address);



THERE EXISTS NO INDIRECT INDEXED X [(d), X], ONLY  indirect indexed Y

Indirect Indexed (d), Y: Take the 8-bit value, and zero page it. After that
you read the value at the zero-paged byte and then read another byte at the address after that one.
Once you get the two bytes from the addresses, you add what's in the Y register to that address
to get the final address, then you read that.
Example: LDA $20 -> 
         low = read((0x20) & 0xFF)
         high = read((0x20 + 1) & 0xFF)
         address = (high << 8 | low) + Y
         read(address)

Example 2: 

  uint8_t initial_val = read(PC);
  PC++;
  uint8_t low = read(initial_val & 0xFF);
  uint8_t high = read((initial_val + 1) & 0xFF);
  
  uint16_t init_addr = (high << 8 | low);
  uint16_t address = (init_addr + Y);

  if ((init_addr & 0xFF00) != (address & 0xFF00)) {
    cycles += 1;
  }

  uint8_t value = read(address);

  ------------------------------------------


  Other addressing modes:

  Accumulator: apply operation directly on the accumulator or A.

  Relative: branch somewhere

  implied: The instruction doesn't require any bytes
  or data. It just executes instructions without any info


*/