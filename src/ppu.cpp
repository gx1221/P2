#include "ppu.h"
#include "cpu.h"
#include "input.h"

/* Pixel processing unit (PPU) which runs independently of the CPU, 
but still time-bound to the same timer and the APU */

PPU::PPU() {
  write_latch = false;
  NMI = false;
  frame_toggle = false;
  ppu_cycles = 0;
  scanline = 0;

  control = 0;
  mask = 0;
  status = 0;
  oam_addr = 0;
  scroll = 0;
  vram_addr = 0;
  t_addr = 0;
  buffer = 0;
  status = 0;
  mapper = nullptr;

  memset(pattern_table, 0, sizeof(pattern_table));
  memset(name_tables,    0, sizeof(name_tables));
  memset(palette_RAM,    0, sizeof(palette_RAM));
  memset(OAM,            0, sizeof(OAM));

  // clear framebuffer
  for (int y = 0; y < 240; ++y) {
    for (int x = 0; x < 256; ++x) {
      framebuffer[y][x] = 0x00000000u; // black
    }
  }

}

void PPU::connectCPU(CPU* cpu_ref) {
  cpu = cpu_ref;
}

void PPU::connectInput(Input* input_ref) {
  input = input_ref;
}

void PPU::connectMapper(Mapper* mapper_ptr) {
  mapper = mapper_ptr;
}

void PPU::write_register(uint16_t cpu_addr, uint8_t value) {
  switch (cpu_addr % 8) {
    case 0: // $2000 - PPUCTRL
      control = value;
      break;
    case 1: // $2001 - PPUMASK
      mask = value;
      break;
    case 2: // $2002 - PPUSTATUS
      // ignore since there's no write
      break;
    case 3: // $2003 - OAMADDR
      oam_addr = value;
      break;
    case 4: // $2004 - OAMDATA
      OAM[oam_addr] = value;
      oam_addr++;
      break;
    case 5: // $2005 - PPUSCROLL
      if (!write_latch) {
          // first write = X scroll
          write_latch = true;
      } else {
          // second write = Y scroll
          write_latch = false;
      }
      break;
    case 6: // $2006 - PPUADDR
      if (!write_latch) {
          t_addr = (t_addr & 0x00FF) | ((value & 0x3F) << 8);
          write_latch = true;
      } else {
          t_addr = (t_addr & 0xFF00) | value;
          vram_addr = t_addr;
          write_latch = false;
      }
      break;
    case 7: // $2007 - PPUDATA
      // write to VRAM

      if (vram_addr < 0x2000) {
          // Pattern table write
          mapper->write_ppu(vram_addr, value);
      } else if (vram_addr >= 0x2000 && vram_addr < 0x3F00) {
          // Nametable write
          mapper->write_ppu(vram_addr, value);
      } else if (vram_addr >= 0x3F00 && vram_addr < 0x4000) {
          // Palette RAM write (handled internally)
          palette_RAM[(vram_addr - 0x3F00) % 32] = value;
      }

      vram_addr += (control & 0x04) ? 32 : 1;
      break;
  }
}


uint8_t PPU::read_register(uint16_t cpu_addr) {
    uint8_t data = 0;
    switch (cpu_addr % 8) {
        case 2: // $2002 - PPUSTATUS
            data = status;
            status &= ~0x80;   // clear vblank flag
            write_latch = false;
            break;
        case 4: // $2004 - OAMDATA
            data = OAM[oam_addr];
            break;
        case 7: // $2007 - PPUDATA
            if (vram_addr < 0x3F00) {
                // buffered read for VRAM
                data = buffer;
                buffer = mapper->read_ppu(vram_addr);
            } else {
                // direct palette read
                data = palette_RAM[(vram_addr - 0x3F00) % 32];
            }

            vram_addr += (control & 0x04) ? 32 : 1;
            break;
    }
    return data;
}

void PPU::oam_write(uint8_t byte) {
  OAM[oam_addr] = byte;
  oam_addr++;
}


void PPU::tick() {
    ppu_cycles++;
    render();
    if (ppu_cycles > 340) { // check if ppu cycles is at end of line
        ppu_cycles = 0;
        scanline++;
        
        // Start of vblank
        if (scanline == 241) {
            status |= FLAG_NEGATIVE; 
            if (control & FLAG_NEGATIVE) {   
              NMI = true;
            }
        }

        // End of vblank
        if (scanline > 261) {
          scanline = 0;
          status &= ~FLAG_NEGATIVE;
          NMI = false;
          frame_toggle = !frame_toggle;
    }
    }
}

void PPU::render() {
  // Make sure that pixels outside visible area aren't rendered
  if (scanline >= 240 || ppu_cycles < 1 || ppu_cycles > 256) {
    return;
  }

  int x = ppu_cycles - 1; // 0 - 255
  int y = scanline; // 0 - 239


  // Rendering background section

  // starting positions of every tile
  int tile_x = x / 8;  
  int tile_y = y / 8;

  uint16_t name_index = tile_y * 32 + tile_x;
  uint16_t nt_addr = 0x2000 + (name_index % 0x400);
  uint8_t tile_number = mapper->read_ppu(nt_addr);

  // Get tile data and tile row info
  uint16_t tile_addr = tile_number * 16 + (y % 8);
  uint8_t low  = mapper->read_ppu(tile_addr);
  uint8_t high = mapper->read_ppu(tile_addr + 8);

  // Get pixel info inside of tile row
  int pixel_x = x % 8;
  uint8_t bit0 = (low  >> (7 - pixel_x)) & 1;
  uint8_t bit1 = (high >> (7 - pixel_x)) & 1;
  uint8_t color_index = (bit1 << 1) | bit0; //get 2-bit color data

  uint32_t color = 0xFF000000; // Initialize to black

  // map color
  if (color_index != 0) {
      uint8_t palette_entry = mapper->read_ppu(0x3F00 + color_index);
      color = nesColor(palette_entry);
  }

  framebuffer[y][x] = color; //actually draw the color

  // Rendering sprites
  for (int sprite = 0; sprite < 64; sprite++) {
      uint8_t sprite_y = OAM[sprite * 4 + 0]; // y position
      uint8_t tile = OAM[sprite * 4 + 1]; // pattern table index
      uint8_t attr = OAM[sprite * 4 + 2]; // attribute
      uint8_t sprite_x = OAM[sprite * 4 + 3]; // x position

      // Check if this pixel is inside the sprite
      if (y >= sprite_y && y < sprite_y + 8 && x >= sprite_x && x < sprite_x + 8) {
          int sx = x - sprite_x;
          int sy = y - sprite_y;

          // Get tile data for sprite
          uint16_t tile_addr = tile * 16 + sy;
          uint8_t low  = mapper->read_ppu(tile_addr);
          uint8_t high = mapper->read_ppu(tile_addr + 8);

          uint8_t bit0 = (low  >> (7 - sx)) & 1;
          uint8_t bit1 = (high >> (7 - sx)) & 1;
          uint8_t sprite_color_index = (bit1 << 1) | bit0;

          if (sprite_color_index != 0) {
              // Select sprite palette index (0x3F10 - 0x3F1F)
              uint8_t palette_entry = mapper->read_ppu(0x3F10 + (attr & 0x03) * 4 + sprite_color_index);
              framebuffer[y][x] = nesColor(palette_entry); // Draw sprite pixel 
          }
      }
  }
}


// Color pallete function
uint32_t PPU::nesColor(uint8_t idx) {
    static const uint32_t nesColors[64] = {
        0x666666,0x002A88,0x1412A7,0x3B00A4,0x5C007E,0x6E0040,0x6C0700,0x561D00,
        0x333500,0x0B4800,0x005200,0x004F08,0x00404D,0x000000,0x000000,0x000000,
        0xADADAD,0x155FD9,0x4240FF,0x7527FE,0xA01ACC,0xB71E7B,0xB53120,0x994E00,
        0x6B6D00,0x388700,0x0E9300,0x008F32,0x007C8D,0x000000,0x000000,0x000000,
        0xFFFEFF,0x64B0FF,0x9290FF,0xC676FF,0xF36AFF,0xFE6ECC,0xFE8170,0xEA9E22,
        0xBCBE00,0x88D800,0x5CE430,0x45E082,0x48CDDE,0x4F4F4F,0x000000,0x000000,
        0xFFFEFF,0xC0DFFF,0xD3D2FF,0xE8C8FF,0xFBC2FF,0xFEC4EA,0xFECCC5,0xF7D8A5,
        0xE4E594,0xCFEF96,0xBDF4AB,0xB3F3CC,0xB5EBF2,0xB8B8B8,0x000000,0x000000
    };
    return 0xFF000000 | nesColors[idx % 64];
}



bool PPU::getNMI() {
  return NMI;
}

void PPU::setNMI(bool val) {
  NMI = val;
}

