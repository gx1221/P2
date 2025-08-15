#include "cpu.h"

CPU::CPU() {
  A = 0x0;
  X = 0x0;
  Y = 0x0;
  SP = 0xFD;
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

uint8_t CPU::read(uint16_t address) const {
  if (address >= 0x2000 && address < 0x4000) {
    //call ppu function
  }
  return CPU_memory[address]; 
}

void CPU::write(uint16_t address, uint8_t value) { 
  if (address >= 0x2000 && address < 0x4000) {
      //call ppu function
  }
  CPU_memory[address] = value; 
}

void CPU::push(uint8_t value) {
  CPU_memory[0x100 + SP] = value;
  SP--;  // Decrement SP after push
  assert((0x100 + SP) >= 0x100);
}


uint8_t CPU::pop() {
  SP++;  // increment SP on pop
  assert((0x100 + SP) <= 0x1FF);
  return CPU_memory[0x100 + SP];
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

uint8_t CPU::fetch() { //fetch 16 bits because opcode can go up to the 
  assert(CPU_memory);
  assert(!bad_instruction);
  uint8_t opcode = CPU::read(PC);
  PC++; //read opcode, then increment PC
  return opcode;
} 


void CPU::decode_and_execute(uint8_t opcode) {
  //uint8_t opcode = fetch();
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

  
// SIGH, I didn't realize until halfway through implementing the
// opcodes that I could just put the process of putting finding
// the values in memory of each addressing mode into a function
// but now it's too late unless I want to rework a bunch of stuff




/*
 *  ADC -  Add with Carry
 *  Add memory flag and carry value to accumulator
 *  C - Carry 	result > $FF 	(If the result overflowed past $FF (wrapping around), unsigned overflow occurred.)
 *  Z - Zero 	result == 0 	
 *  V - Overflow 	(result ^ A) & (result ^ memory) & $80 	(If the result's sign is different from both A's and memory's, signed overflow (or underflow) occurred.)
 *  N - Negative 	result bit 7
 */


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
    uint8_t base_address = read(PC);
    PC++;
    uint8_t address = (base_address + X) & 0xFF; // zero page so mask it with first 256 bit. 
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
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;

  uint16_t address = (high << 8) | low; // 00hi -> hi00 -> hilo puts first byte into beginning and adds low byte using or since it takes over last 8 bits
  uint8_t value = read(address);
  
  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0);

  set_flag(FLAG_CARRY, result > 0xFF);
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); 

  A = result & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);

  cycles += 4;
}

void CPU::adc_absolute_x() {
  uint8_t low = read(PC); 
  uint8_t high = read(PC + 1); 

  PC += 2;

  uint16_t address = (high << 8) | low; 

  // Add a cycle the page of the address + X isn't the same as the original address
  if ((address & 0xFF00) != ((address + X) & 0xFF00)) {
    cycles += 1; 
  }

  uint8_t value = read(address + X); // Add value in x register ot current value and read


  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0);

  set_flag(FLAG_CARRY, result > 0xFF);
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); 

  A = result & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);

  cycles += 4;
  
}

void CPU::adc_absolute_y() {
  uint8_t low = read(PC); 
  uint8_t high = read(PC + 1); 

  PC += 2;

  uint16_t address = (high << 8) | low; 

  // Add a cycle the page of the address + Y isn't the same as the original address
  if ((address & 0xFF00) != ((address + Y) & 0xFF00)) {
    cycles += 1; 
  }

  uint8_t value = read(address + Y); // Add value in y register ot current value and read


  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0);

  set_flag(FLAG_CARRY, result > 0xFF);
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); 

  A = result & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);

  cycles += 4;
  
}

void CPU::adc_indexed_indirect() {
  uint8_t initial_val = read(PC); //get initial value/address
  PC++;

  uint8_t low = read((initial_val + X) & 0xFF);  //take the initial value, add X, then zero page
  uint8_t high = read((initial_val + X + 1) & 0xFF); //get the same thing, but go up 1 to next address

  uint16_t address = high << 8 | low; //combine

  uint8_t value = read(address);

  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0);
  set_flag(FLAG_CARRY, result > 0xFF);
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); 

  A = result & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);

  cycles += 6;
}

void CPU::adc_indirect_indexed() {
  uint8_t initial_val = read(PC); //take initial value
  PC++;

  uint8_t low = read(initial_val & 0xFF); //get the zero paged value
  uint8_t high = read((initial_val + 1) & 0xFF); //get the next value and then zero page it
  
  uint16_t init_addr = (high << 8 | low); //combine bytes

  uint16_t address = (init_addr + Y); // Add Y to the address for this addressing mode

  if ((init_addr & 0xFF00) != (address & 0xFF00)) { //check if first byte changed
    cycles += 1;
  }

  uint8_t value = read(address);
  uint16_t result = A + value + ((P & FLAG_CARRY)? 1 : 0);
  
  
  set_flag(FLAG_CARRY, result > 0xFF);
  set_flag(FLAG_OVERFLOW, ((result ^ A) & (result ^ value) & 0x80)); 

  A = result & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);

  cycles += 5;
}




/*
 *  AND - Bitwise AND
 *  A = A & memory. (This ANDs a memory value and the accumulator)
 * 
 *  Z - Zero 	result == 0
 *  N - Negative  result bit 7 
 */






void CPU::and_immediate() {
  uint8_t value = read(PC);
  PC++;

  A = (A & value) & 0xFF; // Performing the and with value and accumulator
  // still zero-paging just in case even if redundant

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);
  cycles += 2;
} 

void CPU::and_zeropage() {
  uint8_t base_address = read(PC);
  PC++;
  
  uint8_t value = read(base_address);
  A = (A & value) & 0xFF; 
  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);
  cycles += 3;
}


void CPU::and_zeropage_x() {
  uint8_t base_address = read(PC);
  uint8_t value = (base_address + X);

  A = (A & value) & 0xFF;
  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);
  cycles += 4;
}

void CPU::and_absolute() {
  uint8_t low = read(PC);
  uint8_t high = read(PC + 1);
  
  uint16_t address = (high << 8) | low;
  uint8_t value = read(address);

  A = (A & value) & 0xFF;
  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);
  cycles += 4;
};

void CPU::and_absolute_x() {

  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 

  PC += 2;

  uint16_t address = (high << 8) | low; 

  // Page cross
  if ((address & 0xFF00) != ((address + X) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }

  uint8_t value = read(address + X); // Add value in x register to current value and read
  A = (A & value) & 0xFF; // AND value, to accumulator, then set to 
  

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);
  cycles += 4;

}

void CPU::and_absolute_y() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 

  PC += 2;

  uint16_t address = (high << 8) | low; 

  // Page cross
  if ((address & 0xFF00) != ((address + Y) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }

  uint8_t value = read(address + Y); // Add value in x register to current value and read
  A = (A & value) & 0xFF; // AND value, to accumulator, then set to 


  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);
  cycles += 4;

}

void CPU::and_indexed_indirect() {
  uint8_t initial_val = read(PC); //get initial value from PC
  PC++;

  uint8_t low = read((initial_val + X) & 0xFF); //add X to value and zero page
  uint8_t high = read((initial_val + X + 1) & 0xFF); //get add + X + 1 and zero page to get second byte

  uint16_t address = high << 8 | low;

  uint8_t value = read(address);

  A = (A & value) & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);
  cycles += 6;

}

void CPU::and_indirect_indexed() {
  uint8_t initial_val = read(PC);
  PC++;

  uint8_t low = read(initial_val & 0xFF); //get zero paged address
  uint8_t high = read((initial_val + 1) & 0xFF); //get next address and zero page
  
  uint16_t init_addr = (high << 8 | low);
  uint16_t address = (init_addr + Y); // add Y for this mode

  if ((init_addr & 0xFF00) != (address & 0xFF00)) { //check if intial address has the same first byte as new address
    cycles += 1;
  }

  uint8_t value = read(address);

  A = (A & value) & 0xFF;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, A & 0x80);
  cycles += 5;
}




/*
 * ASL - Arithmetic Shift Left
 * value = value << 1 (ASL shifts all of the bits of a memory value or the accumulator one position to the left)
 * 
 * C - Carry 	value bit 7
 * Z - Zero 	result == 0
 * N - Negative 	result bit 7 
 *
 */



void CPU::asl_accumulator() {
  uint8_t carried_bit = (A >> 7); // get first bit/ bit-7 for carry flag
  A <<= 1; // just left shift by 1
  

  set_flag(FLAG_CARRY, carried_bit); // check if there's a 1 or 0 at the bit that was shifted out
  set_flag(FLAG_ZERO, A == 0); // check if A == 0
  set_flag(FLAG_NEGATIVE, (A & 0x80)); // Check if negative flag is on

  cycles += 2;
}


void CPU::asl_zeropage() {
  uint8_t address = read(PC); // get the address
  PC++;

  uint8_t value = read(address); // read value
  uint8_t carried_bit = (value >> 7); // save carried bit

  value <<= 1; //shift value.

  write(address, value); //write back / replace value at memory address

  set_flag(FLAG_CARRY, carried_bit);
  set_flag(FLAG_ZERO, value == 0); //replaced A with value since we're working with the value instead of the accumulator now
  set_flag(FLAG_NEGATIVE, (value & 0x80));
  
  cycles += 5;

}
void CPU::asl_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; //add x to address then zero page
  uint8_t value = read(address);
  
  uint8_t carried_bit = (value >> 7);

  value <<= 1;

  write(address, value);

  set_flag(FLAG_CARRY, carried_bit);
  set_flag(FLAG_ZERO, value == 0);
  set_flag(FLAG_NEGATIVE, (value & 0x80));
  
  cycles += 6;

}
void CPU::asl_absolute() {
  uint8_t low = read(PC); //low byte
  uint8_t high = read(PC + 1);  //high byte

  PC += 2;

  uint16_t address = (high << 8) | low;
  uint8_t value = read(address);

  uint8_t carried_bit = (value >> 7);

  value <<= 1;

  write(address, value);

  set_flag(FLAG_CARRY, carried_bit);
  set_flag(FLAG_ZERO, value == 0);
  set_flag(FLAG_NEGATIVE, (value & 0x80));
  
  cycles += 6;

}
void CPU::asl_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  address += X;
  uint8_t value = read(address);  //read address with X added
  
  // No need for page crossing validation for this for some reason

  uint8_t carried_bit = (value >> 7);
  value <<= 1;

  write(address, value);

  set_flag(FLAG_CARRY, carried_bit);
  set_flag(FLAG_ZERO, value == 0);
  set_flag(FLAG_NEGATIVE, (value & 0x80));
  
  cycles += 7;
}


/* 
 * BCC - Branch if Carry clear
 * 
 * If the carry flag is clear, then branch to a nearby location
 * by adding a relative offset to the PC 
 * The offset is signed and has a range of [-128, 127] 
 * relative to the first byte after the branch instruction
 * 
 * PC = PC + 2 + memory (signed) 
 */ 
void CPU::bcc_relative() {
  if (P & FLAG_CARRY) { //checking if carry flag is on or off
    cycles += 2; // only need to add 2 cycles if no branching/carry flag is on
    PC++;
  }
  else { 
    int8_t offset = static_cast<int8_t>(read(PC)); // get 8-bit offset from value and cast to get full range
    PC++;

    if ((PC & 0xFF00) != ((PC + offset) & 0xFF00)) { //checking for page crossing
      cycles += 1;  
    }

    PC += offset;
    cycles += 3;
  }
}

/*
 * BCS - Branch if Carry Set
 * Same functionality as BCC, but checks for a set carry flag
 * PC = PC + 2 + memory (signed)
 */

void CPU::bcs_relative() {
  if (!(P & FLAG_CARRY)) { //checking if carry flag is on or off
    cycles += 2; // only need to add 2 cycles if no branching/ carry flag is off
    PC++;
  }
  else { 
    int8_t offset = static_cast<int8_t>(read(PC)); // get 8-bit offset from value and cast to get full range
    PC++;

    if ((PC & 0xFF00) != ((PC + offset) & 0xFF00)) { //checking for page crossing
      cycles += 1;  
    }

    PC += offset;
    cycles += 3;
  }
}

/*
 * BCS - Branch if Equal
 * If the zero flag is set, BEQ branches to a nearby location by
 * adding the branch offset to the program counter.
 * PC = PC + 2 + memory (signed)
 */

void CPU::beq_relative() {
  if ((P & FLAG_ZERO) == 0) { //don't branch if the zero flag is clear
    cycles += 2;
    PC++;
  }
  else { //if zero flag is set, then branch
    int8_t offset = static_cast<int8_t>(read(PC)); // get 8-bit offset from value and cast to get full range
    PC++;

    if ((PC & 0xFF00) != ((PC + offset) & 0xFF00)) { //checking for page crossing
      cycles += 1;  
    }

    PC += offset;
    cycles += 3;
  }

}

/*
 * BIT - Bit Test
 * BIT modifies flags, but does not change memory or registers.
 * overflow and negative flags will depend on the memory 
 * zero flag will depend on whether ((A & memory) == 0)
 * Z - Zero 	result == 0
 * V - Overflow 	memory bit 6
 * N - Negative 	memory bit 7 
 */

void CPU::bit_zeropage() {
  uint16_t address = read(PC); //get byte/address at PC
  PC++;

  uint8_t value = read(address); // get the value from address

  set_flag(FLAG_OVERFLOW, (value & FLAG_OVERFLOW));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));

  uint8_t result = A & value;

  set_flag(FLAG_ZERO, (result == 0));

  cycles += 3;

}

void CPU::bit_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);

  set_flag(FLAG_OVERFLOW, (value & FLAG_OVERFLOW));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));

  uint8_t result = A & value;

  set_flag(FLAG_ZERO, (result == 0));

  cycles += 4;

}

/*
 * BMI - Branch if Minus
 * If the negative flag is set, BMI branches to a 
 * nearby location by adding the branch offset to the program counter. 
 * PC = PC + 2 + memory (signed)
 */

void CPU::bmi_relative() {
  if ((P & FLAG_NEGATIVE) == 0) { //don't branch if negative flag is clear
    cycles += 2;
    PC++;
  }
  else { //branch if negative flag is set
    int8_t offset = static_cast<int8_t>(read(PC)); // get 8-bit offset from value and cast to get full range
    PC++;

    if ((PC & 0xFF00) != ((PC + offset) & 0xFF00)) { //checking for page crossing
      cycles += 1;  
    }

    PC += offset;
    cycles += 3;

  } 

}

/*
 * BNE - Branch if Not Equal
 * If the zero flag is clear, BME branches to a 
 * nearby location by adding the branch offset to the program counter. 
 * PC = PC + 2 + memory (signed)
 */

void CPU::bne_relative() {
  if ((P & FLAG_ZERO) != 0) { //don't branch if zero flag is set
    cycles += 2;
    PC++;
  }
  else { // branch if zero flag is set
    int8_t offset = static_cast<int8_t>(read(PC)); // get 8-bit offset from value and cast to get full range
    PC++;

    if ((PC & 0xFF00) != ((PC + offset) & 0xFF00)) { //checking for page crossing
      cycles += 1;  
    }

    PC += offset;
    cycles += 3;

  } 
}

/*
 * BPL - Branch if Plus
 * If the negative flag is clear, BME branches to a 
 * nearby location by adding the branch offset to the program counter. 
 * PC = PC + 2 + memory (signed)
 */

void CPU::bpl_relative() {
  if ((P & FLAG_NEGATIVE) != 0) { //don't branch if negative flag is set
    cycles += 2;
    PC++;
  }
  else { // branch if negative flag is set
    int8_t offset = static_cast<int8_t>(read(PC)); // get 8-bit offset from value and cast to get full range
    PC++;

    if ((PC & 0xFF00) != ((PC + offset) & 0xFF00)) { //checking for page crossing
      cycles += 1;  
    }

    PC += offset;
    cycles += 3;

  } 
}

/*
 * BRK - Break (software IRQ)
 * BRK triggers an interrupt request (IRQ).
 * it takes the BRK opcode and a padding byte that is still consumed as a single instruction
 * then it has to push the address of next instruction after BRK to resume
 * operation after the interrupt is handled 
 * 
 * push PC + 2 to stack
 * push NV11DIZC flags to stack
 * PC = ($FFFE) 
 */
void CPU::brk_immediate() {
  PC += 1; //we're on the value/padding, so now skip the padding 

  //get the address... Remember that you're pushing 8-bit values to memory
  
  push((PC >> 8) & 0xFF);  //Push high byte
  push(PC & 0xFF); //Push low byte

  push((P | FLAG_BREAK));

  set_flag(FLAG_INTERRUPT, true);
  
  // read the vector addresses, which is stored in two bytes
  // in 0xFFFE and 0xFFFF.
  uint8_t low = read(0xFFFE);
  uint8_t high = read(0xFFFF);

  
  PC = (high << 8) | low;

  cycles += 7;
};

void CPU::brk_implied() {
  
  //We don't do anything to the PC in implied 

  //get the address... Remember that you're pushing 8-bit values to memory
  
  push((PC >> 8) & 0xFF);  //Push high byte
  push(PC & 0xFF); //Push low byte

  push((P | FLAG_BREAK));

  set_flag(FLAG_INTERRUPT, true);
  
  // read the vector addresses, which is stored in two bytes
  // in 0xFFFE and 0xFFFF.
  uint8_t low = read(0xFFFE);
  uint8_t high = read(0xFFFF);

  
  PC = (high << 8) | low;

  cycles += 7;
}

/*
 * BVC - Branch if Overflow Clear
 * If the overflow flag is clear, 
 * BVC branches to a nearby location by adding 
 * the branch offset to the program counter. 
 * 
 * PC = PC + 2 + memory (signed)
 */
void CPU::bvc_relative() {
  if ((P & FLAG_OVERFLOW) != 0) { //if overflow is set, don't branch
    cycles += 2;
    PC++;
  }
  else { // If overflow flag is clear, branch.
    int8_t offset = static_cast<int8_t>(read(PC)); // get 8-bit offset from value and cast to get full range
    PC++;

    if ((PC & 0xFF00) != ((PC + offset) & 0xFF00)) { //checking for page crossing
      cycles += 1;  
    }

    PC += offset;
    cycles += 3;

  }
}

/*
 * BVS - Branch if Overflow Set
 * If the overflow flag is set, 
 * BVS branches to a nearby location by adding 
 * the branch offset to the program counter. 
 * 
 * PC = PC + 2 + memory (signed)
 */
void CPU::bvs_relative() {
 if ((P & FLAG_OVERFLOW) == 0) { //if overflow is clear, don't branch
    cycles += 2;
    PC++;
  }
  else { // If overflow flag is set, branch.
    int8_t offset = static_cast<int8_t>(read(PC)); // get 8-bit offset from value and cast to get full range
    PC++;

    if ((PC & 0xFF00) != ((PC + offset) & 0xFF00)) { //checking for page crossing
      cycles += 1;  
    }

    PC += offset;
    cycles += 3;
  }
}


/*
 * CLC - Clear Carry
 * Clear the carry flag
 * 
 * C = 0
 */
void CPU::clc_implied() {
  set_flag(FLAG_CARRY, false);
  cycles +=2;
}

/*
 * CLD - Clear Decimal
 * Clear the decimal flag
 * 
 * D = 0
 */

void CPU::cld_implied() {
  set_flag(FLAG_DECIMAL, false);
  cycles +=2;
}

/*
 * CLI - Clear Interrupt Disable
 * Clear the Interrupt disable flag flag
 * 
 * I = 0
 */
void CPU::cli_implied() {
  set_flag(FLAG_INTERRUPT, false);
  cycles +=2;
}

/*
 * CLD - Clear Overflow
 * Clear the overflow flag
 * 
 * V = 0
 */
void CPU::clv_implied() {
  set_flag(FLAG_OVERFLOW, false);
  cycles +=2;
}


/*
 * CMP - Compare Accumulator
 * CMP compares A to a memory value, setting flags 
 * as appropriate but not modifying any registers. 
 * The comparison is implemented as a subtraction
 * 
 * A - memory (Don't modify the accumulator. Use temp var to compare)
 * 
 * C - Carry 	    A >= memory
 * Z - Zero 	    A == memory
 * N - Negative 	result bit 7 
 */
void CPU::cmp_immediate() {
  uint8_t value = read(PC);
  PC++;
  uint8_t tmp_result = (A - value); //we use a temp var for comparison

  set_flag(FLAG_CARRY, (A >= value)); //compare original accumulator value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 2;
}


void CPU::cmp_zeropage() {
  uint8_t base_address = read(PC);
  PC++;
  
  uint8_t value = read(base_address);
  uint8_t tmp_result = (A - value);
  set_flag(FLAG_CARRY, (A >= value)); //compare original accumulator value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 3;
}
void CPU::cmp_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; //add x to address then zero page
  uint8_t value = read(address);
  uint8_t tmp_result = (A - value);
  set_flag(FLAG_CARRY, (A >= value)); //compare original accumulator value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 4;
}
void CPU::cmp_absolute() {
  uint8_t low = read(PC);
  uint8_t high = read(PC + 1);

  PC +=2;

  uint16_t address = ((high << 8) | low);
  uint8_t value = read(address);
  uint8_t tmp_result = (A - value);
  set_flag(FLAG_CARRY, (A >= value)); //compare original accumulator value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 4;
}
void CPU::cmp_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 

  if ((address & 0xFF00) != ((address + X) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }

  address += X; // Add value of X to address
  uint8_t value = read(address); //read new address
  uint8_t tmp_result = (A - value);
  set_flag(FLAG_CARRY, (A >= value)); //compare original accumulator value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 4;

}
void CPU::cmp_absolute_y() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 

  if ((address & 0xFF00) != ((address + Y) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }

  address += Y; // Add value of Y to address
  uint8_t value = read(address);
  uint8_t tmp_result = (A - value);
  set_flag(FLAG_CARRY, (A >= value)); //compare original accumulator value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 4;

}


void CPU::cmp_indexed_indirect() {
  uint8_t initial_val = read(PC);  //read initial value
  PC++;

  // take initial value and get the high and low bytes using X
  uint8_t low = read((initial_val + X) & 0xFF);
  uint8_t high = read((initial_val + X + 1) & 0xFF);

  uint16_t address = high << 8 | low; //combine bytes to get true address

  uint8_t value = read(address);
  uint8_t tmp_result = (A - value);
  set_flag(FLAG_CARRY, (A >= value)); //compare original accumulator value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 6;
}


void CPU::cmp_indirect_indexed() {
  uint8_t initial_val = read(PC);
  PC++;

  // Get zero paged bytes of high and low bytes
  uint8_t low = read(initial_val & 0xFF);
  uint8_t high = read((initial_val + 1) & 0xFF);
  
  uint16_t init_addr = (high << 8 | low);
  uint16_t address = (init_addr + Y); //add Y to address

  // account for page crossing
  if ((init_addr & 0xFF00) != (address & 0xFF00)) {
    cycles += 1;
  }

  uint8_t value = read(address);
  uint8_t tmp_result = (A - value);
  set_flag(FLAG_CARRY, (A >= value)); //compare original accumulator value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 5;
}

/*
 * CPX - Compare X
 * CPX compares X to a memory value, setting flags
 * as appropriate but not modifying any registers.
 * The comparison is implemented as a subtraction
 * 
 * X - memory (Don't modify the X. Use temp var to compare)
 * 
 * C - Carry 	    X >= memory
 * Z - Zero 	    X == memory
 * N - Negative 	result bit 7 
 */
void CPU::cpx_immediate() {
  uint8_t value = read(PC);
  PC++;
  uint8_t tmp_result = (X - value); //we use a temp var for comparison

  set_flag(FLAG_CARRY, (X >= value)); //compare original X value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 2;
}

void CPU::cpx_zeropage() {
  uint8_t base_address = read(PC);
  PC++;
  
  uint8_t value = read(base_address);
  uint8_t tmp_result = (X - value);
  set_flag(FLAG_CARRY, (X >= value)); //compare original X value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 3;
}

void CPU::cpx_absolute() {
  uint8_t low = read(PC);
  uint8_t high = read(PC + 1);

  PC += 2;

  uint16_t address = ((high << 8) | low);
  uint8_t value = read(address);
  uint8_t tmp_result = (X - value);
  set_flag(FLAG_CARRY, (X >= value)); //compare original X value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 4;
}

/*
 * CPX - Compare Y
 * CPX compares Y to a memory value, setting flags
 * as appropriate but not modifying any registers.
 * The comparison is implemented as a subtraction
 * 
 * Y - memory (Don't modify the Y. Use temp var to compare)
 * 
 * C - Carry 	    Y >= memory
 * Z - Zero 	    Y == memory
 * N - Negative 	result bit 7 
 */
void CPU::cpy_immediate() {
  uint8_t value = read(PC);
  PC++;
  uint8_t tmp_result = (Y - value); //we use a temp var for comparison

  set_flag(FLAG_CARRY, (Y >= value)); //compare original Y value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 2;
}

void CPU::cpy_zeropage() {
  uint8_t base_address = read(PC);
  PC++;
  
  uint8_t value = read(base_address);
  uint8_t tmp_result = (Y - value);
  set_flag(FLAG_CARRY, (Y >= value)); //compare original Y value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 3;
}

void CPU::cpy_absolute() {
  uint8_t low = read(PC);
  uint8_t high = read(PC + 1);

  PC +=2;

  uint16_t address = ((high << 8) | low);
  uint8_t value = read(address);
  uint8_t tmp_result = (Y - value);
  set_flag(FLAG_CARRY, (Y >= value)); //compare original Y value with new value from memory
  set_flag(FLAG_ZERO, (tmp_result == 0)); //check if result == 0
  set_flag(FLAG_NEGATIVE, (tmp_result & FLAG_NEGATIVE)); //check if negative bit of result is active
  cycles += 4;
}

/*
 * DEC - Decrement Memory
 * DEC subtracts 1 from a memory location.
 * Then it writes the new value back into the
 * memory location
 * 
 * memory = memory - 1
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::dec_zeropage() {
  uint8_t base_address = read(PC);
  PC++;
  
  uint8_t value = read(base_address);
  uint8_t result = value - 1;
  write(base_address, result);

  set_flag(FLAG_ZERO, (result == 0));
  set_flag(FLAG_NEGATIVE, (result & FLAG_NEGATIVE));
  cycles += 5;
}
void CPU::dec_zeropage_x() {
  uint8_t base_address = read(PC);
  PC++;

  uint8_t address = (base_address + X) & 0xFF; //add x to address then zero page
  uint8_t value = read(address);
  uint8_t result = value - 1;
  write(address, result);

  set_flag(FLAG_ZERO, (result == 0));
  set_flag(FLAG_NEGATIVE, (result & FLAG_NEGATIVE));
  cycles += 6;
}
void CPU::dec_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  uint8_t result = value - 1;
  write(address, result);

  set_flag(FLAG_ZERO, (result == 0));
  set_flag(FLAG_NEGATIVE, (result & FLAG_NEGATIVE));
  cycles += 6;
}
void CPU::dec_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // No need for page cross in this one.
  address += X;
  uint8_t value = read(address);
  uint8_t result = value - 1;
  write(address, result);

  set_flag(FLAG_ZERO, (result == 0));
  set_flag(FLAG_NEGATIVE, (result & FLAG_NEGATIVE));
  cycles += 7;
}

/*
 * DEX - Decrement X
 * DEX subtracts 1 from X
 * Then it writes the new value back into X.
 * 
 * X = X - 1
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::dex_implied() {
  X -= 1;
  set_flag(FLAG_ZERO, (X == 0));
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 2;
}

/*
 * DEY - Decrement Y
 * DEY subtracts 1 from Y
 * Then it writes the new value back into Y.
 * 
 * Y = Y - 1
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::dey_implied() {
  X -= 1;
  set_flag(FLAG_ZERO, (X == 0));
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 2;
}


/*
 * EOR - Bitwise Exclusive OR
 * EOR exclusive-ORs a memory value and the accumulator
 * then puts the new value back into the accumulator.
 * 
 * A = A ^ memory 
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::eor_immediate() {
  uint8_t value = read(PC);
  PC++;

  A = (A ^ value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));

  cycles += 2;
}

void CPU::eor_zeropage() {
  uint8_t base_address = read(PC);
  PC++;
  
  uint8_t value = read(base_address);
  A = (A ^ value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));

  cycles += 3;
}
void CPU::eor_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; //add x to address then zero page
  uint8_t value = read(address);
  A = (A ^ value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));

  cycles += 4;
}
void CPU::eor_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  A = (A ^ value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));

  cycles += 4;
}
void CPU::eor_absolute_x() {
  uint8_t low = read(PC);
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross
  if ((address & 0xFF00) != ((address + X) & 0xFF00)) { 
    cycles += 1; 
  }
  address += X;
  uint8_t value = read(address);
  A = (A ^ value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));

  cycles += 4;
}
void CPU::eor_absolute_y() {
  uint8_t low = read(PC);
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross
  if ((address & 0xFF00) != ((address + Y) & 0xFF00)) { 
    cycles += 1; 
  }
  address +=Y;
  uint8_t value = read(address);
  A = (A ^ value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));

  cycles += 4;
}

void CPU::eor_indexed_indirect() {
  uint8_t initial_val = read(PC); 
  PC++;

  uint8_t low = read((initial_val + X) & 0xFF);
  uint8_t high = read((initial_val + X + 1) & 0xFF);

  uint16_t address = high << 8 | low;

  uint8_t value = read(address);
  A = (A ^ value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 6;
}
void CPU::eor_indirect_indexed() {
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
  A = (A ^ value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 5;
}

/*
 * INC - Increment Memory
 *
 * INC adds 1 to a memory location
 * 
 * memory = memory + 1
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */

void CPU::inc_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  uint8_t value = read(address);
  uint8_t result = (value + 1); //increment value of memory
  write(address, result); //store new value back into memory

  set_flag(FLAG_ZERO, (result == 0));
  set_flag(FLAG_NEGATIVE, (result & FLAG_NEGATIVE));

  cycles += 5;
}
void CPU::inc_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; //add x to address then zero page
  uint8_t value = read(address);
  uint8_t result = (value + 1);
  write(address, result);

  set_flag(FLAG_ZERO, (result == 0));
  set_flag(FLAG_NEGATIVE, (result & FLAG_NEGATIVE));

  cycles += 6;
}
void CPU::inc_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; // 00hi -> hi00 -> hilo puts first byte into beginning and adds low byte using or since it takes over last 8 bits
  uint8_t value = read(address);
  uint8_t result = (value + 1);
  write(address, result);

  set_flag(FLAG_ZERO, (result == 0));
  set_flag(FLAG_NEGATIVE, (result & FLAG_NEGATIVE));

  cycles += 6;
}
void CPU::inc_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // no need for page cross check in this
  address += X;
  uint8_t value = read(address);
  uint8_t result = (value + 1);
  write(address, result);

  set_flag(FLAG_ZERO, (result == 0));
  set_flag(FLAG_NEGATIVE, (result & FLAG_NEGATIVE));

  cycles += 7;
}

/*
 * INX - Increment X
 *
 * INX adds 1 to the X register.
 * Note that it does not affect carry nor overflow. 
 * 
 * X = X + 1
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */

void CPU::inx_implied() {
  X++;
  set_flag(FLAG_ZERO, (X == 0));
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 2;
}

/*
 * INX - Increment Y
 *
 * INX adds 1 to the Y register.
 * Note that it does not affect carry nor overflow. 
 * 
 * Y = Y + 1
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::iny_implied() {
  Y++;
  set_flag(FLAG_ZERO, (Y == 0));
  set_flag(FLAG_NEGATIVE, (Y & FLAG_NEGATIVE));
}

/*
 * JMP - Jump
 *
 * JMP sets the program counter to a new value, 
 * allowing code to execute from a new location
 * 
 * PC = memory
 */

void CPU::jmp_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 

  PC = address;
  cycles += 3;
}

void CPU::jmp_indirect() {
  /* 
   * jmp indirect is a special addressing mode
   * where instead of just getting the address
   * directly from the bytes after the opcode,
   * you take the address, read it and treat it
   * as the low byte, then read the (address + 1)
   * and treat it as the high byte
   * 
   * Also, there's a bug where if the low byte
   * ends in FF and crosses a page, then the 
   * 6602 CPU will stay in that same page
   * 
   * Example: JMP ($03FF) reads $03FF (low byte) and $0300 (high byte) instead of $0400
   * since the high byte would be PC + 1, it should go
   * from $03FF to $0400, but instead it just wraps back around the page to $0300
   * 
   */
   
  
  uint8_t low  = read(PC);       // low byte
  uint8_t high = read(PC + 1);   // high byte
  uint16_t base_address = (high << 8) | low; 
  PC += 2;
  
  uint16_t address = 0; //just making an address variable

  // We're trying to implement the bug since it was like this in the original

  if (low == 0xFF) { //if the byte is maxxed out, keep the page the same
    address = (read(base_address & 0xFF00) << 8) | read(base_address); 
  }
  else { //if the byte is not maxxed out, go to the next page
    address = (read(base_address + 1) << 8) | read(base_address);
  }

  PC = address;
  cycles += 5;
}

/*
 * JSR - Jump to Subroutine
 *
 * JSR pushes the current program counter to the 
 * stack and then sets the program counter to a new value.
 * 
 * 
 * push PC + 2 to stack
 * PC = memory 
 */

void CPU::jsr_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  
  // According to the manual, in JSR, the return address on the stack points 1 byte
  // before the start of the next instruction
  uint16_t return_address = (PC - 1); 

  push((return_address >> 8) & 0xFF); //push first 8 bits (high)
  push(return_address & 0xFF);  //push last 8 bits (low)

  PC = address;
  cycles += 6;
}

/*
 * LDA - Load A
 *
 * LDA loads a memory value into the accumulator. 
 * 
 * A = memory 
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */

void CPU::lda_immediate() {
  uint8_t value = read(PC);
  A = value;
  PC++;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 2;
}
void CPU::lda_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  uint8_t value = read(address);
  A = value;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 3;
}
void CPU::lda_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; //add x to address then zero page
  uint8_t value = read(address);
  A = value;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 4;
}
void CPU::lda_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  A = value;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 4;
}
void CPU::lda_absolute_x() {
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
  A = value;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 4;
}
void CPU::lda_absolute_y() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross
  if ((address & 0xFF00) != ((address + Y) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }
  address += Y;
  uint8_t value = read(address);  //read address with X added
  A = value;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 4;
}
void CPU::lda_indexed_indirect() {
  uint8_t initial_val = read(PC); 
  PC++;

  uint8_t low = read((initial_val + X) & 0xFF);
  uint8_t high = read((initial_val + X + 1) & 0xFF);

  uint16_t address = high << 8 | low;

  uint8_t value = read(address);
  A = value;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 6;
}

void CPU::lda_indirect_indexed() {
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
  A = value;

  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 5;
}

/*
 * LDX - Load X
 *
 * LDX loads a memory value into X. 
 * 
 * X = memory 
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */

void CPU::ldx_immediate() {
  uint8_t value = read(PC);
  PC++;

  X = value;
  set_flag(FLAG_ZERO, X == 0);
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 2;
}
void CPU::ldx_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  uint8_t value = read(address);
  X = value;
  set_flag(FLAG_ZERO, X == 0);
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 3;
}
void CPU::ldx_zeropage_y() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + Y) & 0xFF; //add x to address then zero page
  uint8_t value = read(address);
  X = value;
  set_flag(FLAG_ZERO, X == 0);
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 4;
}
void CPU::ldx_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  X = value;
  set_flag(FLAG_ZERO, X == 0);
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 4;

}
void CPU::ldx_absolute_y() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross
  if ((address & 0xFF00) != ((address + Y) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }
  address += Y;
  uint8_t value = read(address);  //read address with Y added
  
  X = value;
  set_flag(FLAG_ZERO, X == 0);
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 4;
}

/*
 * LDY - Load Y
 *
 * LDY loads a memory value into Y. 
 * 
 * Y = memory 
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */

void CPU::ldy_immediate() {
  uint8_t value = read(PC);
  PC++;

  Y = value;
  set_flag(FLAG_ZERO, Y == 0);
  set_flag(FLAG_NEGATIVE, (Y & FLAG_NEGATIVE));
  cycles += 2;
}
void CPU::ldy_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  uint8_t value = read(address);
  Y = value;
  set_flag(FLAG_ZERO, Y == 0);
  set_flag(FLAG_NEGATIVE, (Y & FLAG_NEGATIVE));
  cycles += 3;
}
void CPU::ldy_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; //add x to address then zero page
  uint8_t value = read(address);
  Y = value;
  set_flag(FLAG_ZERO, Y == 0);
  set_flag(FLAG_NEGATIVE, (Y & FLAG_NEGATIVE));
  cycles += 4;
}
void CPU::ldy_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  Y = value;
  set_flag(FLAG_ZERO, Y == 0);
  set_flag(FLAG_NEGATIVE, (Y & FLAG_NEGATIVE));
  cycles += 4;
}
void CPU::ldy_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross
  if ((address & 0xFF00) != ((address + X) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }
  address += X;
  uint8_t value = read(address); 
  Y = value;
  set_flag(FLAG_ZERO, Y == 0);
  set_flag(FLAG_NEGATIVE, (Y & FLAG_NEGATIVE));
  cycles += 4;
}

/*
 * LSR - Logical Shift Right
 *
 * LSR shifts all of the bits of a memory value or 
 * the accumulator one position to the right, 
 * moving the value of each bit into the next bit. 
 * 0 is shifted into bit 7, and bit 0 is shifted 
 * into the carry flag
 * 
 * This is different from the arithmetic shift because
 * it just treats the value like unsigned. Just doing
 * a regular bitshift
 * 
 * value = value >> 1
 * 
 * C - Carry 	    value bit 0
 * Z - Zero 	    result == 0
 * N - Negative 	0 
 */

void CPU::lsr_accumulator() {
  uint8_t carried_bit = (A & 0x01); //save shifted bit to check for carry
  A >>= 1; // left shift once
  set_flag(FLAG_CARRY, carried_bit); //bit 0 shifted to carry flag
  set_flag(FLAG_ZERO, (A == 0)); // check if A == 0
  set_flag(FLAG_NEGATIVE, 0); // Unsigned is never negative, so always deactivate negative flag
  cycles += 2;
}
void CPU::lsr_zeropage() {
  uint8_t address = read(PC);
  PC++;

  uint8_t value = read(address);
  uint8_t carried_bit = (value & 0x01);
  uint8_t result = (value >> 1);

  write(address, result);

  set_flag(FLAG_CARRY, carried_bit);
  set_flag(FLAG_ZERO, (result == 0)); 
  set_flag(FLAG_NEGATIVE, 0);
  cycles += 5;
}
void CPU::lsr_zeropage_x() {
  uint8_t base_address = read(PC); 
  PC++;

  uint8_t address = (base_address + X) & 0xFF; 
  uint8_t value = read(address);
  uint8_t carried_bit = (value & 0x01);
  uint8_t result = (value >> 1);
  write(address, result);
  set_flag(FLAG_CARRY, carried_bit);
  set_flag(FLAG_ZERO, (result == 0)); 
  set_flag(FLAG_NEGATIVE, 0);
  cycles += 6;
}
void CPU::lsr_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  uint8_t carried_bit = (value & 0x01);
  uint8_t result = (value >> 1);
  write(address, result);
  set_flag(FLAG_CARRY, carried_bit);
  set_flag(FLAG_ZERO, (result == 0)); 
  set_flag(FLAG_NEGATIVE, 0);

  cycles += 6;
}
void CPU::lsr_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross isn't needed
  address += X;
  uint8_t value = read(address);  //read address with X added
  uint8_t carried_bit = (value & 0x01);
  uint8_t result = (value >> 1);
  write(address, result);
  set_flag(FLAG_CARRY, carried_bit);
  set_flag(FLAG_ZERO, (result == 0)); 
  set_flag(FLAG_NEGATIVE, 0);
  cycles += 7;
}

/*
 * NOP - No Operation
 *
 * NOP has no effect; it merely wastes space 
 * and CPU cycles
 */
void CPU::nop_implied() {
  cycles += 2;
}

/*
 * ORA - Bitwise OR
 *
 * ORA inclusive-ORs a memory value and the accumulator, 
 * bit by bit. If either input bit is 1, the resulting bit 
 * is 1. Otherwise, it is 0. 
 * 
 * A = A | memory
 * 
 * Z - Zero     	result == 0
 * N - Negative 	result bit 7 
 */

void CPU::ora_immediate() {
  uint8_t value = read(PC);
  PC++;
  A = (A | value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 2;
}
void CPU::ora_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  uint8_t value = read(address);
  A = (A | value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 3;
}
void CPU::ora_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; 
  uint8_t value = read(address);
  A = (A | value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 4;
}
void CPU::ora_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  A = (A | value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 4;
}
void CPU::ora_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  if ((address & 0xFF00) != ((address + X) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }
  address += X;
  uint8_t value = read(address);
  A = (A | value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));

  cycles += 4;
}
void CPU::ora_absolute_y() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  if ((address & 0xFF00) != ((address + Y) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }
  address += Y;
  uint8_t value = read(address);
  A = (A | value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));

  cycles += 4;
}

void CPU::ora_indexed_indirect() {
  uint8_t initial_val = read(PC); 
  PC++;

  uint8_t low = read((initial_val + X) & 0xFF);
  uint8_t high = read((initial_val + X + 1) & 0xFF);

  uint16_t address = high << 8 | low;

  uint8_t value = read(address);
  cycles += 6;
  A = (A | value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
}
void CPU::ora_indirect_indexed() {
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
  A = (A | value);
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 5;
}

/*
 * PHA - Push A
 *
 * PHA stores the value of A to the current stack 
 * position and then decrements the stack pointer. 
 * 
 * ($0100 + SP) = A
 * SP = SP - 1 
 */
void CPU::pha_implied() {
  push(A);
  // I already deincrement the stack pointer in the push function
  cycles += 3;
}

/*
 * PHP - Push Processor Status
 *
 * PHP stores a byte to the stack containing the 6 
 * status flags and B flag and then decrements the 
 * stack pointer. 
 * 
 * ($0100 + SP) = NV11DIZC
 * SP = SP - 1  
 * 
 * B - Break 	Pushed as 1 	(This flag exists only
 * in the flags byte pushed to the stack, not as 
 * real state in the CPU.) 
 */

void CPU::php_implied() {
    uint8_t copy = P | FLAG_BREAK; // set break flag in copy
    push(copy);
    cycles += 3;
}


/*
 * PLA - Pull A
 *
 * PLA increments the stack pointer and then 
 * loads the value at that stack position into A. 
 * 
 * SP = SP + 1
 * A = ($0100 + SP) 
 * 
 * Z - Zero 	result == 0
 * N - Negative 	result bit 7 
 */
void CPU::pla_implied() {
  //I already increment the SP inside the pop() function 
  A = pop();
  set_flag(FLAG_ZERO, A == 0);
  set_flag(FLAG_NEGATIVE, (A & 0x80));
  cycles += 4;
}

/*
 * PLP - Pull Processor Status
 *
 * PLP increments the stack pointer and then loads 
 * the value at that stack position into the 6 
 * status flags
 * 
 * SP = SP + 1
 * NVxxDIZC = ($0100 + SP) 
 * 
 * C - Carry 	            result bit 0 	
 * Z - Zero 	            result bit 1 	
 * I - Interrupt         	result bit 2
 * D - Decimal          	result bit 3 	
 * V - Overflow 	        result bit 6 	
 * N - Negative         	result bit 7 	
 */
void CPU::plp_implied() {
  // I already increment the stack pointer in pop()
  P = pop();
  P &= ~(FLAG_BREAK); //clear the break flag
  // maybe a bug later, but i could set the extra flag too
  cycles += 4;
}

/*
 * ROL - Rotate Left
 *
 * ROL shifts a memory value or the accumulator to the 
 * left, moving the value of each bit into the next bit 
 * and treating the carry flag as though it is both above 
 * bit 7 and below bit 0
 * 
 * basically copy the carry flag, 
 * shift the left-most bit into the current carry flag
 * shift the copied carry flag into bit 0
 * 
 * value = value << 1 through C
 * 
 * C - Carry    	value bit 7
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::rol_accumulator() {
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (A >> 7)); //shift bit 7 into carry flag
  A <<= 1; //left shift by 1
  A |= old_carry; //put value of old carry flag into 0th bit
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 2;
}
void CPU::rol_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  uint8_t value = read(address);
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (value>> 7)); //shift bit 7 into carry flag
  value <<= 1;
  value |= old_carry;
  write(address, value);
  set_flag(FLAG_ZERO, (value == 0));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));
  cycles += 5;
}
void CPU::rol_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; 
  uint8_t value = read(address);
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (value>> 7)); //shift bit 7 into carry flag
  value <<= 1;
  value |= old_carry;
  write(address, value);
  set_flag(FLAG_ZERO, (value == 0));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));
  cycles += 6;
}
void CPU::rol_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (value>> 7)); //shift bit 7 into carry flag
  value <<= 1;
  value |= old_carry;
  write(address, value);
  set_flag(FLAG_ZERO, (value == 0));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));
  cycles += 6;
}
void CPU::rol_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // No need to check page cross
  address += X;
  uint8_t value = read(address);
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (value>> 7)); //shift bit 7 into carry flag
  value <<= 1;
  value |= old_carry;
  write(address, value);
  set_flag(FLAG_ZERO, (value == 0));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));
  cycles += 7;
}

/*
 * ROR - Rotate Right
 *
 * ROR shifts a memory value or the accumulator to the 
 * right, moving the value of each bit into the next bit 
 * and treating the carry flag as though it is both above 
 * bit 7 and below bit 0
 * 
 * Same concept as ROL, but everything circularly rolls to
 * the right instead of left
 * 
 * value = value >> 1 through C
 * 
 * C - Carry    	value bit 7
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::ror_accumulator() {
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (A & 0x01)); //shift bit 0 into carry flag
  A >>= 1; //right shift by 1
  A |= (old_carry << 7); //put value of old carry flag into 7th bit
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 2;
}
void CPU::ror_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  uint8_t value = read(address);
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (value & 0x01)); //shift bit 0 into carry flag
  value >>= 1;
  value |= (old_carry << 7);
  write(address, value);
  set_flag(FLAG_ZERO, (value == 0));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));
  cycles += 5;
}
void CPU::ror_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; 
  uint8_t value = read(address);
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (value & 0x01)); //shift bit 0 into carry flag
  value >>= 1;
  value |= (old_carry << 7);
  write(address, value);
  set_flag(FLAG_ZERO, (value == 0));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));
  cycles += 6;
}
void CPU::ror_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (value & 0x01)); //shift bit 0 into carry flag
  value >>= 1;
  value |= (old_carry << 7);
  write(address, value);
  set_flag(FLAG_ZERO, (value == 0));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));
  cycles += 6;
}
void CPU::ror_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // No need to check page cross
  address += X;
  uint8_t value = read(address);
  uint8_t old_carry = (P & FLAG_CARRY); //store old carry flag
  set_flag(FLAG_CARRY, (value & 0x01)); //shift bit 0 into carry flag
  value >>= 1;
  value |= (old_carry << 7);
  write(address, value);
  set_flag(FLAG_ZERO, (value == 0));
  set_flag(FLAG_NEGATIVE, (value & FLAG_NEGATIVE));
  cycles += 7;
}

/*
 * RTI - Return from Interrupt
 *
 * RTI returns from an interrupt handler, 
 * first pulling the 6 status flags from the stack and 
 * then pulling the new program counter. The flag 
 * pulling behaves like PLP except that changes to the 
 * interrupt disable flag apply immediately instead of 
 * being delayed 1 instruction.
 * 
 * pull NVxxDIZC flags from stack
 * pull PC from stack 
 * 
 * C - Carry    	pulled flags bit 0 	
 * Z - Zero 	    pulled flags bit 1 	
 * I - Interrupt 	pulled flags bit 2
 * D - Decimal   	pulled flags bit 3 	
 * V - Overflow 	pulled flags bit 6 	
 * N - Negative 	pulled flags bit 7 	 
 */
void CPU::rti_implied() {
  P = pop();
  P &= ~(FLAG_BREAK); //clear the break flag
  // since PC is 16 bits, we pop the high and low bytes then combine
  uint8_t pc_low = pop(); 
  uint8_t pc_high = pop();
  PC = (pc_high << 8) | pc_low;
  cycles += 6;
}

/*
 * RTS - Return from Subroutine
 *
 * RTS pulls an address from the stack into the 
 * program counter and then increments the program 
 * counter.
 * 
 * pull PC from stack
 * PC = PC + 1 
 */
void CPU::rts_implied() {
  uint8_t pc_low = pop(); 
  uint8_t pc_high = pop();
  PC = (pc_high << 8) | pc_low;
  PC++;
  cycles += 6;  
}

/*
 * SBC - Subtract with Carry
 *
 * SBC subtracts a memory value and the bitwise NOT of 
 * carry from the accumulator.
 * 
 * A = A - memory - ~C, 
 * or equivalently: A = A + ~memory + C 
 * 
 * C - Carry 	    ~(result < $00)
 * Z - Zero 	    result == 0 	
 * V - Overflow 	(result ^ A) & (result ^ ~memory) & $80 
 * N - Negative 	result bit 7
 */
void CPU::sbc_immediate() {
  uint8_t value = read(PC);
  PC++;

  uint16_t temp = A + (~value) + (P & FLAG_CARRY ? 1 : 0); //need this to be exactly 1 when adding
  set_flag(FLAG_CARRY, temp > 0xFF); 
  set_flag(FLAG_ZERO, (uint8_t)temp == 0);
  set_flag(FLAG_OVERFLOW, ((A ^ temp) & (A ^ ~value) & 0x80));
  set_flag(FLAG_NEGATIVE, (temp & 0x80));

  A = (uint8_t)temp;
  cycles += 2;
}

void CPU::sbc_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  uint8_t value = read(address);
  uint16_t temp = A + (~value) + (P & FLAG_CARRY ? 1 : 0); 
  set_flag(FLAG_CARRY, temp > 0xFF); 
  set_flag(FLAG_ZERO, (uint8_t)temp == 0);
  set_flag(FLAG_OVERFLOW, ((A ^ temp) & (A ^ ~value) & 0x80));
  set_flag(FLAG_NEGATIVE, (temp & FLAG_NEGATIVE));

  A = (uint8_t)temp;
  cycles += 3;
}
void CPU::sbc_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; 
  uint8_t value = read(address);
  uint16_t temp = A + (~value) + (P & FLAG_CARRY ? 1 : 0); 
  set_flag(FLAG_CARRY, temp > 0xFF); 
  set_flag(FLAG_ZERO, (uint8_t)temp == 0);
  set_flag(FLAG_OVERFLOW, ((A ^ temp) & (A ^ ~value) & 0x80));
  set_flag(FLAG_NEGATIVE, (temp & FLAG_NEGATIVE));

  A = (uint8_t)temp;

  cycles += 4;

}
void CPU::sbc_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  uint8_t value = read(address);
  uint16_t temp = A + (~value) + (P & FLAG_CARRY ? 1 : 0); 
  set_flag(FLAG_CARRY, temp > 0xFF); 
  set_flag(FLAG_ZERO, (uint8_t)temp == 0);
  set_flag(FLAG_OVERFLOW, ((A ^ temp) & (A ^ ~value) & 0x80));
  set_flag(FLAG_NEGATIVE, (temp & FLAG_NEGATIVE));

  A = (uint8_t)temp;
  cycles += 4;
}
void CPU::sbc_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross
  if ((address & 0xFF00) != ((address + X) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }
  address += X;
  uint8_t value = read(address);
  uint16_t temp = A + (~value) + (P & FLAG_CARRY ? 1 : 0); 
  set_flag(FLAG_CARRY, temp > 0xFF); 
  set_flag(FLAG_ZERO, (uint8_t)temp == 0);
  set_flag(FLAG_OVERFLOW, ((A ^ temp) & (A ^ ~value) & 0x80));
  set_flag(FLAG_NEGATIVE, (temp & FLAG_NEGATIVE));

  A = (uint8_t)temp;
  cycles += 4;
}
void CPU::sbc_absolute_y() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross
  if ((address & 0xFF00) != ((address + Y) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }
  address += Y;
  uint8_t value = read(address);
  uint16_t temp = A + (~value) + (P & FLAG_CARRY ? 1 : 0); 
  set_flag(FLAG_CARRY, temp > 0xFF); 
  set_flag(FLAG_ZERO, (uint8_t)temp == 0);
  set_flag(FLAG_OVERFLOW, ((A ^ temp) & (A ^ ~value) & 0x80));
  set_flag(FLAG_NEGATIVE, (temp & FLAG_NEGATIVE));

  A = (uint8_t)temp;
  cycles += 4;
}
void CPU::sbc_indexed_indirect() {
  uint8_t initial_val = read(PC); 
  PC++;

  uint8_t low = read((initial_val + X) & 0xFF);
  uint8_t high = read((initial_val + X + 1) & 0xFF);

  uint16_t address = high << 8 | low;

  uint8_t value = read(address);
  uint16_t temp = A + (~value) + (P & FLAG_CARRY ? 1 : 0); 
  set_flag(FLAG_CARRY, temp > 0xFF); 
  set_flag(FLAG_ZERO, (uint8_t)temp == 0);
  set_flag(FLAG_OVERFLOW, ((A ^ temp) & (A ^ ~value) & 0x80));
  set_flag(FLAG_NEGATIVE, (temp & FLAG_NEGATIVE));

  A = (uint8_t)temp;
  cycles += 6;
}
void CPU::sbc_indirect_indexed() {
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
  uint16_t temp = A + (~value) + (P & FLAG_CARRY ? 1 : 0); 
  set_flag(FLAG_CARRY, temp > 0xFF); 
  set_flag(FLAG_ZERO, (uint8_t)temp == 0);
  set_flag(FLAG_OVERFLOW, ((A ^ temp) & (A ^ ~value) & 0x80));
  set_flag(FLAG_NEGATIVE, (temp & FLAG_NEGATIVE));

  A = (uint8_t)temp;

  cycles += 5;
}

/*
 * SEC - Set Carry
 *
 * SEC sets the carry flag. In particular, 
 * this is usually done before subtracting the 
 * low byte of a value with SBC to avoid subtracting an 
 * extra 1. 
 * 
 * C = 1
 * 
 * C - Carry   1
 */
void CPU::sec_implied() {
  set_flag(FLAG_CARRY, 1);
  cycles += 2;
}

/*
 * SED - Set Decimal
 *
 * SED sets the Decimal flag.
 * 
 * D = 1
 * 
 * D - Decimal   1
 */
void CPU::sed_implied() {
  set_flag(FLAG_DECIMAL, 1);
  cycles += 2;
}

/*
 * SEI - Set Interrupt Disable
 *
 * SEI sets the Interrupt disable flag.
 * 
 * I = 1
 * 
 * I - Interrupt disable   1
 */
void CPU::sei_implied() {
  set_flag(FLAG_INTERRUPT, 1);
  cycles += 2;
}

/*
 * STA - Store A
 *
 * STA stores the accumulator value into memory. 
 * 
 * memory = A
 */

void CPU::sta_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  write(address, A);
  cycles += 3;
}
void CPU::sta_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; 
  write(address, A);
  cycles += 4;
}
void CPU::sta_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  write(address, A);
  cycles += 4;
}
void CPU::sta_absolute_x() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 
  // Page cross
  if ((address & 0xFF00) != ((address + X) & 0xFF00)) { //checking if first byte is the same as original after adding
    cycles += 1; 
  }
  address += X;
  write(address, A);
  cycles += 5;
}
void CPU::sta_absolute_y() {
  uint8_t low = read(PC); //read two bytes on pc
  uint8_t high = read(PC + 1); 
  PC += 2;
  uint16_t address = (high << 8) | low; 

  address += Y;
  write(address, A);
  cycles += 5;
}
void CPU::sta_indexed_indirect() {
  uint8_t initial_val = read(PC); 
  PC++;

  uint8_t low = read((initial_val + X) & 0xFF);
  uint8_t high = read((initial_val + X + 1) & 0xFF);

  uint16_t address = high << 8 | low;
  write(address, A);
  cycles += 6;
} 
void CPU::sta_indirect_indexed() {
  uint8_t initial_val = read(PC);
  PC++;
  uint8_t low = read(initial_val & 0xFF);
  uint8_t high = read((initial_val + 1) & 0xFF);
  
  uint16_t init_addr = (high << 8 | low);
  uint16_t address = (init_addr + Y);
  write(address, A);
  cycles += 6;
}

/*
 * STX - Store X
 *
 * STX stores the X value into memory. 
 * 
 * memory = X
 */

void CPU::stx_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  write(address, X);
  cycles += 3;
}
void CPU::stx_zeropage_y() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + Y) & 0xFF; 
  write(address, X);
  cycles += 4;
}
void CPU::stx_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  write(address, X);
  cycles += 4;
}

/*
 * STY - Store Y
 *
 * STY stores the Y value into memory. 
 * 
 * memory = Y
 */

void CPU::sty_zeropage() {
  uint8_t address = read(PC);
  PC++;
  
  write(address, Y);
  cycles += 3;
}
void CPU::sty_zeropage_x() {
  uint8_t base_address = read(PC); //get initial address
  PC++;

  uint8_t address = (base_address + X) & 0xFF; 
  write(address, Y);
  cycles += 4;
}
void CPU::sty_absolute() {
  uint8_t low = read(PC);  // first byte of address
  uint8_t high = read(PC + 1); // second byte of address

  PC += 2;
  uint16_t address = (high << 8) | low; 
  write(address, Y);
  cycles += 4;
}

/*
 * TAX - Transfer A to X
 *
 * TAX copies the accumulator value to the X register. 
 * 
 * X = A
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::tax_implied() {
  X = A;
  set_flag(FLAG_ZERO, (X == 0));
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 2;
}

/*
 * TAY - Transfer A to Y
 *
 * TAY copies the accumulator value to the Y register. 
 * 
 * Y = A
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::tay_implied() {
  Y = A;
  set_flag(FLAG_ZERO, (Y == 0));
  set_flag(FLAG_NEGATIVE, (Y & FLAG_NEGATIVE));
  cycles += 2;
}

/*
 * TSX - Transfer Stack Pointer to X
 *
 * TSX copies the stack pointer value to the X register. 
 * 
 * X = SP
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::tsx_implied() {
  X = SP;
  set_flag(FLAG_ZERO, (X == 0));
  set_flag(FLAG_NEGATIVE, (X & FLAG_NEGATIVE));
  cycles += 2;
}

/*
 * TXA - Transfer X to A
 *
 * TXA copies the X register value to the accumulator.  
 * 
 * A = X
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::txa_implied() {
  A = X;
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 2;
}

/*
 * TXS - Transfer X to Stack Pointer
 *
 * TXS copies the X register value to the stack pointer. 
 * 
 * SP = X
 * 
 */
void CPU::txs_implied() {
  SP = X;
  cycles += 2;
}

/*
 * TYA - Transfer Y to A
 *
 * TYA copies the Y register value to the accumulator.  
 * 
 * A = Y
 * 
 * Z - Zero 	    result == 0
 * N - Negative 	result bit 7 
 */
void CPU::tya_implied() {
  A = Y;
  set_flag(FLAG_ZERO, (A == 0));
  set_flag(FLAG_NEGATIVE, (A & FLAG_NEGATIVE));
  cycles += 2;
}