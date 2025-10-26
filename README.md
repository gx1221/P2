# NES Emulator (C++17 + SDL2)

A simple NES emulator written in **C++17** using **SDL2** for graphics and input handling.  
Supports **Mapper 0 (NROM)** and **Mapper 1 (MMC1)**, with accurate CPU/PPU timing and scroll rendering. 

Can run most NES games like Super Mario Brothers, Legend of Zelda, Donkey Kong, etc.

---

## Features

- **6502 CPU Emulation** with full opcode table  
- **PPU Rendering** with background, sprites, mirroring, and palette support  
- **Mapper0 (NROM)** and **Mapper1 (MMC1)** implementation  
- **Input Handling** using SDL keyboard state  
- **Frame-accurate timing** (CPU:PPU ratio 1:3)  
- Basic **NMI handling** and **framebuffer rendering**

---
## Prerequisites

You’ll need:
- **CMake ≥ 3.10**
- **C++17-compatible compiler** (GCC, Clang, or MSVC)
- **SDL2 development library**

### Installing SDL2

#### Linux / WSL:
```bash
sudo apt update
sudo apt install libsdl2-dev
```

#### Windows:
If using MSYS2:
```bash
pacman -S mingw-w64-x86_64-SDL2
```


---

## Building the Emulator

```bash
# Clone and enter project
git clone https://github.com/yourusername/nes-emulator.git
cd nes-emulator

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make
```

---

## Running

Place your `.nes` ROM files inside the `testing/` directory, e.g.:
```
testing/Super_Mario_Brothers.nes
```

Go to test.cpp and change the directory for loading the rom:
```
cpu->loadROM("testing/Super_Mario_Brothers.nes");
```

Then run:
```bash
./test
```

If you created a custom target in CMake:
```bash
make run
```


---

## Controls

| NES Button | Keyboard Key |
|-------------|--------------|
| A           | `X`          |
| B           | `Z`          |
| Start       | `Enter`      |
| Select      | `Right Shift`|
| D-Pad       | Arrow Keys   |

---

## Future Plans

- Add **APU (Audio Processing Unit)** emulation  
- Support for additional mappers  
- Add **save states** and **controller remapping**  
- Optional **OpenGL** or **ImGui-based** renderer  

---

## 📄 License

This project is open-source under the MIT License.

---
