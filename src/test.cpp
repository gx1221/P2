#include <SDL2/SDL.h>
#include "cpu.h"
#include "ppu.h"

int main() {
    // Create objects dynamically with cross-references
    CPU* cpu = nullptr;
    PPU* ppu = nullptr;

    // First create PPU (with temporary null CPU pointer)
    ppu = new PPU(nullptr);

    // Now create CPU, giving it pointer to PPU
    cpu = new CPU(ppu);

    // Link PPU back to CPU (PPU must store CPU* cpu;)
    ppu->cpu = cpu;

    // Load ROM
    cpu->loadROM("01-basics.nes");

    // Initialize opcode table
    cpu->init_opcode_table();

    // --- SDL2 setup ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "NES Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        256 * 2, 240 * 2,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             256, 240);

    bool running = true;
    SDL_Event e;

    // --- Main loop ---
    const int CPU_CLOCK_HZ = 1789773;                   // NES NTSC CPU
const int CPU_CYCLES_PER_FRAME = CPU_CLOCK_HZ / 60; // ~29830

while (running) {
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
    }

    // Simulate one frame worth of CPU cycles
    uint64_t start_cycles = cpu->get_cycles();
    uint64_t target_cycles = start_cycles + CPU_CYCLES_PER_FRAME;

    while (cpu->get_cycles() < target_cycles) {
        uint64_t before = cpu->get_cycles();
        cpu->step(); // must increment cpu->cycles in instruction handlers
        uint64_t after = cpu->get_cycles();

        uint64_t used = (after - before);
        if (used == 0) used = 1; // safety: avoid stalling if cycle accounting missing

        // For every CPU cycle, tick the PPU 3 times
        for (uint64_t i = 0; i < used * 3; ++i) {
            ppu->tick();

            // If PPU just set NMI, handle it immediately
            if (ppu->getNMI()) {
                ppu->setNMI(false);
                cpu->nmi();
            }
        }
    }

    // Render frame (copy framebuffer once per host frame)
    SDL_UpdateTexture(texture, nullptr, ppu->framebuffer, 256 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    // Small delay to avoid pegging CPU. You can tune or use vsync.
    SDL_Delay(1);
}

    // --- Cleanup ---
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    delete cpu;
    delete ppu;

    return 0;
}
