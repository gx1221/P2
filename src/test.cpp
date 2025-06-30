#include <SDL2/SDL.h>
#include <iostream>

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    std::cout << "Hello, SDL2!" << std::endl;
    SDL_Quit();
    return 0;
}
