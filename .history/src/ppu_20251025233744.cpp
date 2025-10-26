#include "ppu.h"
#include "cpu.h"
#include "input.h"

/* Pixel processing unit (PPU) which runs independently of the CPU, 
but still time-bound to the same timer and the APU */

PPU::PPU() {
  NMI = false;
  frame_toggle = false;
  ppu_cycles = 0;
  scanline = 0;

  control = 0;
  mask = 0;
  status = 0;
  oam_addr = 0;
  vram_addr = 0;      // current VRAM addr
  temp_vram = 0;      // temp VRAM addr
  x = 0;      // fine X scroll
  write_latch = false; // write latch
  buffer = 0;
  status = 0;
  mapper = nullptr;
  

  memset(pattern_table, 0, sizeof(pattern_table));
  memset(name_tables,    0, sizeof(name_tables));
  memset(palette_RAM,    0, sizeof(palette_RAM));
  memset(OAM,            0, sizeof(OAM));

  // clear framebuffer for every dot at every scanline
  for (int y = 0; y < 240; ++y) {
    for (int x = 0; x < 256; ++x) {
    framebuffer[y * 256 + x] = 0x00000000u; // black 
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
      temp_vram = (temp_vram & 0xF3FF) | ((value & 0x03) << 10);
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
      oam_addr = (oam_addr + 1) & 0xFF;
      break;
    case 5: // $2005 - PPUSCROLL
      if (!write_latch) {
        x = value & 0x07;
        temp_vram = (temp_vram & 0xFFE0) | ((value >> 3) & 0x1F);
        write_latch = true;
      } else {
        // temp_vram: yyy NN YYYYY XXXXX
        // fine Y (bits 12 to 14)
        // coarse Y (bits 5 to 9)
        temp_vram = (temp_vram & 0x8C1F) | (((uint16_t)(value & 0x07)) << 12) | (((uint16_t)(value & 0xF8)) << 2);
        write_latch = false;
      }
      break;
    case 6: // $2006 - PPUADDR
  if (!write_latch) {
    temp_vram = (temp_vram & 0x00FF) | (((uint16_t)(value & 0x3F)) << 8);
    write_latch = true;
  } 
  else {
    temp_vram = (temp_vram & 0xFF00) | value;
    // Copy vram if we're not in the rendering fetch windows.
    bool rendering_enabled = (mask & 0x18) != 0;
    bool on_visible_or_prerender = (scanline <= 239) || (scanline == 261);
    bool in_fetch_window = (on_visible_or_prerender &&
                          (((ppu_cycles >= 1) && (ppu_cycles <= 256)) ||
                          ((ppu_cycles >= 321) && (ppu_cycles <= 336))));

    if (!rendering_enabled || !in_fetch_window) {
      vram_addr = temp_vram;
    }
    write_latch = false;
  }
  break;

    case 7: // $2007 - PPUDATA
      uint16_t addr = vram_addr & 0x3FFF;
      if (addr < 0x2000) {
        mapper->write_ppu(addr, value);
      } 
      else if (addr < 0x3F00) {
        mapper->write_ppu(addr, value);
      } 
      else {
        uint16_t pal = (addr - 0x3F00) & 0x1F;
        if ((pal & 0x13) == 0x10) {
          pal &= ~0x10;
        }
        palette_RAM[pal] = value;
      }
      // increment vram_addr by 1 or 32 based on PPUCTRL bit 2
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
        case 7: { // $2007 - PPUDATA
          uint16_t addr = vram_addr & 0x3FFF;
          uint8_t out;

          if (addr < 0x3F00) {
            // buffered read
            out = buffer;
            buffer = mapper->read_ppu(addr);
          } 
          else {
              // palette direct read
              uint16_t pal = (addr - 0x3F00) & 0x1F;
              if ((pal & 0x13) == 0x10) {
                pal &= ~0x10;
              }
              out = palette_RAM[pal];
              uint16_t name_table = (addr - 0x1000) & 0x2FFF;
              buffer = mapper->read_ppu(name_table);
          }

          vram_addr += (control & 0x04) ? 32 : 1;
          return out;
      }
    }
    return data;
}

// literally just write to oam then update address
void PPU::oam_write(uint8_t byte) {
  OAM[oam_addr] = byte;
  oam_addr = (oam_addr + 1) & 0xFF;
}

// increment horizontal scroll in Vram address
void PPU::incX() {
  if ((vram_addr & 0x001F) == 31) {
    vram_addr &= ~0x001F; vram_addr ^= 0x0400;
  }
  else {
    vram_addr += 1;
  }
}

// increment the vertical scroll in vram
void PPU::incY() {
  if ((vram_addr & 0x7000) != 0x7000) {
    vram_addr += 0x1000;
  }
  else {
    vram_addr &= ~0x7000;
    uint16_t y = (vram_addr & 0x03E0);
    if (y == 0x03A0) {
      vram_addr &= ~0x03E0; vram_addr ^= 0x0800;
    }
    else if (y == 0x03E0) {
      vram_addr &= ~0x03E0;
    }
    else {
      vram_addr += 0x20;
    }
  }
}

// copy horizontal scroll bits from temp vram to actual 
void PPU::copyX() {
  // clear bits from vram and insert from temp vram
  vram_addr = (vram_addr & 0xFBE0) | (temp_vram & 0x041F);
}

// copy vertical scroll bits from temp vram to actual 
void PPU::copyY() {
  vram_addr = (vram_addr & 0x841F) | (temp_vram & 0x7BE0);
}

void PPU::tick() {
  ppu_cycles++;

  // VBL start
  if (scanline == 241 && ppu_cycles == 1) {
    status |= PPUSTATUS_VBLANK;
    if (control & PPUCTRL_NMI) NMI = true;
  }

  // Prerender clear status bits
  if (scanline == 261 && ppu_cycles == 1) {
    status &= ~PPUSTATUS_VBLANK;
    status &= ~PPUSTATUS_SPRITE0;
    status &= ~PPUSTATUS_OVERFLOW;
    NMI = false;
  }

  // Drawing pixel before scroll
  render();

  //check if rendering option is on in mask through sprite and background flags 
  bool rendering = ((mask & 0x18) != 0);
  if (rendering) {
    if (scanline <= 239) {

      //horizontal scrolling
      if (ppu_cycles >= 2 && ppu_cycles <= 257) {
        if ((ppu_cycles & 7) == 0) { // 8 cycles an increment
          incX();
        }
      }
      if (ppu_cycles == 256) {
        incY();
      }
      if (ppu_cycles == 257) {
        copyX();
        }
      } 
    else if (scanline == 261) {
      // ONLY do the vertical copy. Do NOT incX at 328/336,
      // because I sampled from vram_addr directly in render().
      if (ppu_cycles >= 280 && ppu_cycles <= 304) {
        copyY();
      }
    }
}


  // Odd-frame skip?
  if ((scanline == 261) && (ppu_cycles == 340) && rendering && frame_toggle) {
    ppu_cycles = 0;
    scanline = 0;
    frame_toggle = !frame_toggle;
    return;
  }

  if (ppu_cycles == 341) { //move to next scan line at end of scan line
    ppu_cycles = 0;
    scanline++;

    if (scanline >= 262) { // go back to first scan line if at end
    scanline = 0;
    frame_toggle = !frame_toggle;
   }
  }
}


void PPU::render() {
  // draw visible area pixels
  if ((scanline >= 240) || (ppu_cycles < 1) || (ppu_cycles > 256)) {
    return;
  }

  int y = scanline;
   
  bool sprEnabled = (mask & 0x10) != 0; // show sprites
  bool sprLeft  = (mask & 0x04) != 0; // show sprites in leftmost 8 px

  bool visibleDot = (scanline < 240) && (ppu_cycles >= 1 && ppu_cycles <= 256);
  int  xdot = ppu_cycles - 1;

  bool bgEnabled = (mask & 0x08) != 0; // check ppu mask to see if background is enabled
  bool bgLeft = (mask & 0x02) != 0; // check if background in leftmost 8 px is enabled
  bool inLeft8 = (xdot < 8);

  // Decoding vram for every dot
  uint8_t coarseY =  ((vram_addr >> 5)  & 0x1F); // Y tile coord
  uint8_t name_table_XBit  =  ((vram_addr >> 10) & 1);
  uint8_t name_table_YBit  =  ((vram_addr >> 11) & 1);
  uint8_t fineY   =  ((vram_addr >> 12) & 7); // Y pixel coord in tile

  // handling x coordiante
  uint8_t coarseX_base = (vram_addr & 0x1F);
  uint8_t groupDot = (xdot & 7); // find where current pixel in tile is being rendered
  uint8_t Tilepx = (this->x + groupDot);   // how many pixels into tile we are
  uint8_t pxInTile = (Tilepx & 7);            // pixel with tile
  uint8_t carryTile = (Tilepx >> 3);           // Check if new tile is crossed

  uint8_t localCoarseX = ((coarseX_base + carryTile) & 31);
  uint8_t pageX = (((coarseX_base + carryTile) >> 5) & 1);
  uint8_t name_table_X = (name_table_XBit ^ pageX);

  // Name table base and tile index
  uint16_t base_nametable = 0x2000 + ((name_table_YBit << 1) | name_table_X) * 0x400;
  uint16_t name_table_addr = base_nametable + (coarseY % 30) * 32 + localCoarseX;
  uint8_t  tile_number = mapper->read_ppu(name_table_addr & 0x3FFF);

  // Get Pattern and pixel select for background
  uint16_t bg_pattern_base = (control & 0x10) ? 0x1000 : 0x0000;
  uint16_t tile_addr = bg_pattern_base + (uint16_t)tile_number * 16 + (uint16_t)fineY;
  
  // get the 16 of tile in pattern table
  uint8_t low = mapper->read_ppu(tile_addr & 0x1FFF);
  uint8_t high = mapper->read_ppu((tile_addr + 8) & 0x1FFF);

  // getting color index bits and combining
  uint8_t bit0 = (low  >> (7 - pxInTile)) & 1;
  uint8_t bit1 = (high >> (7 - pxInTile)) & 1;
  uint8_t color_index = (bit1 << 1) | bit0;

  // Attribute fetch
  uint16_t attr_addr   = (base_nametable + 0x3C0)
                      + (((coarseY % 30) / 4) * 8)
                      + (localCoarseX / 4);
  uint8_t  attr_byte   = mapper->read_ppu(attr_addr & 0x3FFF);

  // getting amount to shift attr_byte and finding which color pallete background tile is
  int shift = (((coarseY % 4) / 2) * 2 + ((localCoarseX % 4) / 2)) * 2;
  uint8_t palette_high_bits = (attr_byte >> shift) & 0x03;

  // --- Apply left-8/bg enable mask to background pixel ---
  uint8_t eff_bg_ci = color_index;
  if (!bgEnabled || (inLeft8 && !bgLeft)) eff_bg_ci = 0;

  // --- Background palette lookup & draw ---
  uint8_t  bg_pal_byte = (eff_bg_ci == 0)
                      ? palette_RAM[0]
                      : palette_RAM[((palette_high_bits << 2) | (eff_bg_ci & 0x03)) & 0x1F];

  framebuffer[y * 256 + xdot] = nesColor(bg_pal_byte & 0x3F);

  // SPRITES
  if (sprEnabled) {
    bool spriteMode8x16 = (control & 0x20) != 0;
    int  spriteHeight   = spriteMode8x16 ? 16 : 8;

    for (int i = 0; i < 64; ++i) {
      // OAM entry
      uint8_t sy_raw = OAM[i*4 + 0];
      uint8_t tile   = OAM[i*4 + 1];
      uint8_t attr   = OAM[i*4 + 2];
      uint8_t sx     = OAM[i*4 + 3];

      // Screen-space test
      int sy = (int)sy_raw + 1;                 // NES quirk: Y is top-1
      if (y < sy || y >= sy + spriteHeight) continue;
      if (xdot < sx || xdot >= sx + 8) continue;

      // Pixel within sprite
      int col = xdot - sx;
      int row = y - sy;
      int px  = (attr & 0x40) ? (7 - col) : col; // horizontal flip

      // Select sprite pattern address
      uint16_t pattern_addr;
      if (spriteMode8x16) {
        uint16_t spr_base = (tile & 1) ? 0x1000 : 0x0000;
        bool     vflip    = (attr & 0x80) != 0;

        bool     topHalf;
        uint8_t  fineY;
        if (!vflip) { topHalf = (row < 8);  fineY = row & 7; }
        else        { topHalf = (row >= 8); fineY = 7 - (row & 7); }

        uint8_t  tileIndex = (tile & 0xFE) + (topHalf ? 0 : 1);
        pattern_addr       = spr_base + (uint16_t)tileIndex * 16 + fineY;
      } else {
        uint16_t spr_base = (control & 0x08) ? 0x1000 : 0x0000;
        int      fineY    = (attr & 0x80) ? (7 - (row & 7)) : (row & 7);
        pattern_addr      = spr_base + (uint16_t)tile * 16 + fineY;
      }

      // Fetch sprite bits
      uint8_t slow   = mapper->read_ppu(pattern_addr & 0x1FFF);
      uint8_t shigh  = mapper->read_ppu((pattern_addr + 8) & 0x1FFF);
      uint8_t sb0    = (slow  >> (7 - px)) & 1;
      uint8_t sb1    = (shigh >> (7 - px)) & 1;
      uint8_t spr_ci = (sb1 << 1) | sb0;

      // Left 8-pixel clipping for sprites
      bool spriteClipped = inLeft8 && !sprLeft;

      // Sprite 0 hit logic (only if both non-transparent at this dot)
      if (i == 0 && visibleDot) {
        bool bgOpaqueForHit  = (eff_bg_ci != 0);
        bool sprOpaqueForHit = (spr_ci != 0) && !spriteClipped;
        if (bgOpaqueForHit && sprOpaqueForHit) status |= PPUSTATUS_SPRITE0;
      }

      // Transparent or clipped? Skip
      if (spr_ci == 0 || spriteClipped) continue;

      // Priority handling (behind background if attr & 0x20)
      bool behind_bg            = (attr & 0x20) != 0;
      bool bg_transparent_here  = (eff_bg_ci == 0);

      if (!behind_bg || bg_transparent_here) {
        // Sprite palette fetch
        uint16_t paddr = 0x3F10 + ((attr & 0x03) << 2) + (spr_ci & 0x03);
        uint16_t pidx  = (paddr - 0x3F00) & 0x1F;
        if ((pidx & 0x13) == 0x10) pidx &= ~0x10; // palette mirrors
        uint8_t pal = palette_RAM[pidx] & 0x3F;

        // Draw sprite pixel
        framebuffer[y * 256 + xdot] = nesColor(pal);
        break; // next sprite (this pixel is resolved)
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

void PPU::set_oam_address(uint8_t addr) {
    oam_addr = addr;
}

