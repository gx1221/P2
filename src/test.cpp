#include <SDL2/SDL.h>
#include <signal.h>
#include "cpu.h"
#include "ppu.h"
#include "input.h"



void handle_exit(int) {
    SDL_Quit();
    exit(0);
}

int main() {
    CPU* cpu;
    PPU* ppu;
    Input* input;

    // create instances of cpu, ppu, and input
    ppu = new PPU();
    cpu = new CPU();
    input = new Input();

    // Load ROM
    cpu->loadROM("testing/legend_of_zelda.nes");
    if (!cpu->mapper) {
        fprintf(stderr, "CPU mapper is null. Mapper might not be implemented.\n");
    } 

    // connect all units to each other
    ppu->connectMapper(cpu->mapper);
    ppu->connectCPU(cpu);
    ppu->connectInput(input);
    cpu->connectPPU(ppu);
    cpu->connectInput(input);

    // Initialize opcode table
    cpu->init_opcode_table();
    
    // Setting up SDL
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
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
    bool running = true;
    SDL_Event e;

    // MAIN LOOP FOR EMULATION

    const int CPU_CLOCK_HZ = 1789773; // Copied this from online idk if implemented the pal version or not
    const int CPU_CYCLES_PER_FRAME = CPU_CLOCK_HZ / 60;
 
    bool prevNMI = false;
    bool currentNMI = false;
    
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }

        // constantly get current key state and update
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        input->update_controller(keys);


        // Simulate one frame worth of CPU cycles
        uint64_t start_cycles = cpu->get_cycles();
        uint64_t target_cycles = start_cycles + CPU_CYCLES_PER_FRAME;

        while (cpu->get_cycles() < target_cycles) {
            uint64_t before = cpu->get_cycles();
            cpu->step();

            uint64_t after = cpu->get_cycles();
            uint64_t used = (after - before);
            if (used == 0) {
                used = 1;
            }

            bool pendingNMI = false;

            // For every CPU cycle, tick the PPU 3 times
            for (uint64_t i = 0; i < used * 3; ++i) {
                ppu->tick();
                bool level = ppu->getNMI();

                if (!currentNMI && level && !prevNMI) {
                    pendingNMI = true;
                }
                prevNMI = level;
            }

            if (pendingNMI) {
                currentNMI = true;
                cpu->nmi();
                for (int i = 0; i < 21; i++) { // 7 cycles * 3 dots
                    ppu->tick();
                    prevNMI = ppu->getNMI();
                }
                currentNMI = false;
            }
        }

        // Render frame (copy framebuffer once per frame)
        SDL_UpdateTexture(texture, nullptr, ppu->framebuffer, 256 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_Delay(1);
    }

        // Clean up
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        delete cpu;
        delete ppu;
        delete input;

        return 0;
}
