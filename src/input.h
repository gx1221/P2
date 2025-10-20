#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

#define BUTTON_A      (1 << 0)
#define BUTTON_B      (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START  (1 << 3)
#define BUTTON_UP     (1 << 4)
#define BUTTON_DOWN   (1 << 5)
#define BUTTON_LEFT   (1 << 6)
#define BUTTON_RIGHT  (1 << 7)


class Input {
public:
    struct NESController {
        uint8_t state = 0;       // Current button states
        uint8_t shift_reg = 0;   // Serial shift register
        bool strobe = false;     // Strobe flag
    };

    NESController controller1;
    NESController controller2;

    void update_controller(const Uint8* keys);
    void write_strobe(uint8_t value);
    uint8_t read_controller1();
    uint8_t read_controller2();
};