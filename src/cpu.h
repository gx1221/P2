#pragma once
#include <iostream>
#include <cstdlib>
#include <stdint.h>

class CPU {
  public:
    void update_timers();
    uint16_t fetch();
    void decode_and_execute(uint16_t);
  private:

    // Accumulator. supports using status register for carrying and overflow detection
    uint8_t A; // 0x0 in constructor

    //Program counter
    uint16_t PC; 

    // X and Y indexes. Loop counters
    uint8_t X;
    uint8_t Y;

    /*
      Stack pointer. The stack itself
      is located inside the memory/RAM,
      In our CPU, our stack grows down, so we start off from
      the highest memory location of the stack and then go down
      Stack pointer points into page 1: 0x0100–0x01FF 256 bytes
      SP itself is just an 8-bit offset from 0x0100 
      so the real location would be memory[0x100 + SP]
      the stack pointer points to the first "Available" memory location
      then goes down to the next available spot to show that it's
      available. For example, If SP = 0xFF, and we push, 0x01FF
      is filled, then SP-- to 0xFE.
    */
    uint8_t SP; //  initialize to 0xFF after reset to start at beginngin 0x01FF

    /*
      Status register. An 8-bit register. Each bit is a flag
      But only 6 of the bits actually are used by the cpu and arithmetic units
      7654 3210
      NV1B DIZC 
      The B flag and 1 do nothing
     */
    uint8_t P;
      





};