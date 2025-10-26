#pragma once
#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include <vector>
#include <SDL2/SDL.h>

#define PPUCTRL 0x2000
#define PPUMASK 0x2001
#define PPUSTATUS 0x2002
#define OAMADDR 0x2003
#define OAMDATA 0x2004
#define PPUSCROLL 0x2005
#define PPUADDR 0x2006
#define PPUDATA 0x2007
#define OAMDMA 0x4014

static constexpr uint8_t PPUCTRL_NMI = 0x80;

// PPUSTATUS bits
static constexpr uint8_t PPUSTATUS_VBLANK   = 0x80; // bit7
static constexpr uint8_t PPUSTATUS_SPRITE0  = 0x40; // bit6
static constexpr uint8_t PPUSTATUS_OVERFLOW = 0x20; // bit5

class CPU; // forward declaration to connect classes
class Input;
class Mapper;
class PPU
{
public:
  PPU();
  CPU* cpu;
  Input* input;
  void connectCPU(CPU*);
  void connectInput(Input*);
  void connectMapper(Mapper*);
  void write_register(uint16_t, uint8_t);
  uint8_t read_register(uint16_t);
  void oam_write(uint8_t);
  void set_oam_address(uint8_t);
  void tick();
  void render();
  bool getNMI();
  void setNMI(bool);
  uint32_t nesColor(uint8_t);
  uint32_t framebuffer[240 * 256]; //buffer to draw image
  
  void incX();
  void incY();
  void copyX();
  void copyY();


private:
    uint8_t control;     // $2000 - PPUCTRL
    uint8_t mask;        // $2001 - PPUMASK
    uint8_t status;      // $2002 - PPUSTATUS
    uint8_t oam_addr;    // $2003 - OAMADDR
    uint8_t buffer;      // read buffer for $2007
    uint16_t ppu_cycles; // cycles of ppu - will be used to draw every pixel/cycle (1-256 pixels)
    uint16_t scanline;   // current line being drawn 10-239 is user visible 240-260 is for other purposes
    bool frame_toggle;   // toggle frame
    bool NMI; //non-maskable interrupt
    uint16_t vram_addr = 0;  // current VRAM address (15 bits)
    uint16_t temp_vram = 0;  // temp VRAM address (15 bits)
    uint8_t  x = 0;  // fine X scroll or literally the x coordinate of current dot (3 bits)
    bool write_latch; // write latch

    uint16_t bg_pattern_base_latched = 0; //base address of background pattern table
    uint8_t  bg_tile_latched = 0; //tile index number
    uint8_t  bg_attr_latched = 0; //attribute byte
    uint8_t  bg_low_latched = 0; //low byte of tile pattern
    uint8_t  bg_high_latched = 0; //high byte of tile pattern

    // Track which coarse X tile we last latched
    int last_latch_tile_x = 0;
    int last_latch_scanline = 0;


  //Remember the pattern tables, name tables, and pallete_ram are all part of the Vram, but not OAM

  // 2 Pattern Tables - (2) * 4 KB = 8 KB
  uint8_t pattern_table[0x2000]; // $0000-$1FFF
  
  //4 Name Tables - (4) * 1 KB = 4 KB
  uint8_t name_tables[0x1000]; //$2000-$2FFF

  //Palette RAM - 32 Bytes
  uint8_t palette_RAM[32]; // $3F00-$3FFF

  //OAM 64 sprites = (64) * 4 bytes = 256 bytes
  uint8_t OAM[256];

  /*  The 4 bytes:
   *  Byte 0 - Y position of top of sprite (sprite data delayed one scanline)
   *  Byte 1 - Tile index number (bits 7-1 for number. bit 0 for bank of tiles)
   *  Byte 2 - Attributes (look in the wiki later)
   *  Byte 3 -  X position of left of sprite
   * */

    Mapper* mapper;
};


/*
 * When reading or writing on an address in the PPU
 * It's modulo'd by 8 to choose any of the 8 registers

 * PPU REGISTERS
 * 
 * PPUCTRL (read) - Address: 0x2000 - Used for misc settings
 * 
 * PPUMASK (write) - Address: 0x2001 - Used for rendering settings
 * 
 * PPUSTATUS (read) - Address: 0x2002 - Used for rendering events
 * 
 * OAMADDR (write) - Address: 0x2003 - Sprite RAM address
 * 
 * OAMDATA (read/write) - Address: 0x2004 - Sprite RAM data
 * 
 * PPUSCROLL (write) - Address: 0x2005 - X and Y scroll
 * 
 * PPUADDR (write) - Address: 0x2006 - VRAM address
 * 
 * PPUDATA (read/write) - Address: 0x2007 - VRAM data
 * 
 * OAMDMA (write) - Address: 0x4014 - Sprite DMA 
 * 
 * Object Attribute Memory (OAM) 
 * determines how sprites are rendered. 
 * The CPU can manipulate this through memory 
 * mapped registers at OAMADDR ($2003), OAMDATA ($2004), 
 * and OAMDMA ($4014). OAM can just be view as an array 
 * with 64 entries. Each entry has 4 bytes: the sprite Y 
 * coordinate, the sprite tile number, the sprite attribute, 
 * and the sprite X coordinate. 
 * 
 */ 