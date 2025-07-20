#include "cpu.h"

CPU::CPU() {
  A = 0x0;
  X = 0x0;
  Y = 0x0;
  SP = 0x0;
  P = 0x0;

  reset_vector = read(0xFFFC) | (read(0xFFFD) << 8);
  PC = reset_vector;

  memset(CPU_memory, 0, sizeof(CPU_memory));
  memset(opcode_table, 0, sizeof(opcode_table));
  bad_instruction = false;
  cycles = 0;

}

void CPU::set_flag(uint8_t flag, bool condition) {
    if (condition)
        P |= flag;
    else
        P &= ~flag;
}

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

void CPU::loadROM(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open ROM: " << filename << std::endl;
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Failed to read ROM." << std::endl;
        return;
    }

    // Load into memory starting at 0x200
    for (int i = 0; i < size; ++i) {
        CPU::write(0x4020 + i, static_cast<uint8_t>(buffer[i]));
    }

    file.close();
}

uint16_t CPU::fetch() {
  assert(CPU_memory);
  assert(!bad_instruction);
  uint8_t opcode = CPU::read(PC);
  PC++; //read opcode, then increment PC
  return opcode;
} 


void CPU::decode_and_execute(uint16_t opcode) {
  //uint8_t opcode = read(PC++);
  //opcode_table[opcode]();
}

//REMEMBER TO SET CYCLES AND FLAGS
/*
void init_opcode_table(void) {
    Clear all entries to a default handler (e.g. illegal opcode)
    for (int i = 0; i < 256; ++i) {
        opcode_table[i] = illegal_instruction;
    }

    Assign valid handlers
    opcode_table[0xA9] = lda_immediate;  // LDA #immediate
    opcode_table[0xA5] = lda_zeropage;   // LDA zeropage
    opcode_table[0x00] = brk;            // BRK
    assign all 151 official opcodes
}
*/


void CPU::illegal_instruction() {
    printf("Illegal opcode 0x%02X at PC=0x%04X\n", read(PC - 1), PC - 1); // Tracking opcode and location
    bad_instruction = true; //
}
  
void CPU::adc_immediate() {
  uint8_t value = read(PC);
  PC++;
  
  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0); // if C flag (0x01) is on, add 1, else nothing
  //also using 16 bits for result to detect overflow
  
  //Carry flag checking logic
  //00000001 & 10001101 = 00000001 Carry ON = 1/Non-zero/true
  //00000001 & 11011110 = 00000000 Carry OFF = 0/false

  set_flag(FLAG_CARRY, result > 0xFF); //check for overflow and set flag
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); //formula to check for negative/positive overflow

  A = result & 0xFF; //since we had 16 bits, we want to go back to 8 bits since A is 8 bits

  //using A instead of result since we want the values to be masked to 8 bits and not 16 from the result.
  set_flag(FLAG_ZERO, A == 0); 
  set_flag(FLAG_NEGATIVE,  A & 0x80); // check if 7th (Negative) bit is on. also a true bool is any non-zero value
  
  cycles += 2;
}

void CPU::adc_zeropage() {
  uint8_t address = read(PC);
  PC++;
  uint8_t value = read(address); //this is only 8 bit (max 256) so no need to modulo to zero page

  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0); // if C flag is on, add 1, else nothing
  set_flag(FLAG_CARRY, result > 0xFF);
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); //formula to check for negative/positive overflow

  A = result & 0xFF; //since we had 16 bits, make sure it's 8 bit maske

  //using A instead of result since we want the values to be masked to 8 bits
  set_flag(FLAG_ZERO, A == 0); 
  set_flag(FLAG_NEGATIVE,  A & 0x80); // check if 7th (Negative) bit is on. also a true bool is any non-zero value
  
  cycles += 3;
}

  void CPU::adc_zeropage_x() {
    uint8_t base_addr = read(PC);
    PC++;
    uint8_t address = (base_addr + X) & 0xFF; // zero page so mask it with first 256 bit. 
    uint8_t value = read(address);

    uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0); // if C flag is on, add 1, else nothing
    set_flag(FLAG_CARRY, result > 0xFF); //set carry flag in P based on if result overflows
    set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); //formula to check for negative/positive overflow

    A = result & 0xFF; //since we had 16 bits, make sure it's 8 bit maske

    //using A instead of result since we want the values to be masked to 8 bits
    set_flag(FLAG_ZERO, A == 0); 
    set_flag(FLAG_NEGATIVE,  A & 0x80); // check if 7th/last bit (Negative) bit is on. also a true bool is any non-zero value
    
    cycles += 4;
}

void CPU::adc_absolute() {
  uint8_t low = read(PC + 1);  // first byte of address
  uint8_t high = read(PC + 2); // second byte of address

  PC += 2;

  uint16_t addr = (high << 8) | low; // 00hi -> hi00 -> hilo puts first byte into beginning and adds low byte using or since it takes over last 8 bits
  uint8_t value = read(addr);
  
  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0);

  set_flag(FLAG_CARRY, result > 0xFF);
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); 

  A = result & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);

  cycles += 4;
}

void CPU::adc_absolute_x() {
  uint8_t low = read(PC + 1); 
  uint8_t high = read(PC + 2); 

  PC += 2;

  uint16_t addr = (high << 8) | low; 

  // Add a cycle the page of the address + X isn't the same as the original address
  if ((addr & 0xFF00) != ((addr + X) & 0xFF00)) {
    cycles += 1; 
  }

  uint8_t value = read(addr + X); // Add value in x register ot current value and read


  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0);

  set_flag(FLAG_CARRY, result > 0xFF);
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); 

  A = result & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);

  cycles += 4;
  
}

void CPU::adc_absolute_y() {
  uint8_t low = read(PC + 1); 
  uint8_t high = read(PC + 2); 

  PC += 2;

  uint16_t addr = (high << 8) | low; 

  // Add a cycle the page of the address + Y isn't the same as the original address
  if ((addr & 0xFF00) != ((addr + Y) & 0xFF00)) {
    cycles += 1; 
  }

  uint8_t value = read(addr + Y); // Add value in y register ot current value and read


  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0);

  set_flag(FLAG_CARRY, result > 0xFF);
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); 

  A = result & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);

  cycles += 4;
  
}

void CPU::adc_indexed_indirect() {
  uint8_t value = read(PC);
  PC++;

  
}

void CPU::adc_indirect_indexed() {


}
