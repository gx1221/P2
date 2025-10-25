// input.cpp
#include "input.h"

void Input::update_controller(const Uint8* keys) {
    controller1.state = 0;
    controller2.state = 0;

    // Player 1 mappings
    if (keys[SDL_SCANCODE_Z]) controller1.state |= BUTTON_A;
    if (keys[SDL_SCANCODE_X]) controller1.state |= BUTTON_B;
    if (keys[SDL_SCANCODE_RSHIFT]) controller1.state |= BUTTON_SELECT;
    if (keys[SDL_SCANCODE_RETURN]) controller1.state |= BUTTON_START;
    if (keys[SDL_SCANCODE_UP]) controller1.state |= BUTTON_UP;
    if (keys[SDL_SCANCODE_DOWN]) controller1.state |= BUTTON_DOWN;
    if (keys[SDL_SCANCODE_LEFT]) controller1.state |= BUTTON_LEFT;
    if (keys[SDL_SCANCODE_RIGHT]) controller1.state |= BUTTON_RIGHT;

    // Player 2 mappings
    if (keys[SDL_SCANCODE_V]) controller2.state |= BUTTON_A;
    if (keys[SDL_SCANCODE_C]) controller2.state |= BUTTON_B;
    if (keys[SDL_SCANCODE_Q]) controller2.state |= BUTTON_SELECT;
    if (keys[SDL_SCANCODE_E]) controller2.state |= BUTTON_START;
    if (keys[SDL_SCANCODE_W]) controller2.state |= BUTTON_UP;
    if (keys[SDL_SCANCODE_S]) controller2.state |= BUTTON_DOWN;
    if (keys[SDL_SCANCODE_A]) controller2.state |= BUTTON_LEFT;
    if (keys[SDL_SCANCODE_D]) controller2.state |= BUTTON_RIGHT;
}


void Input::write_strobe(uint8_t value) {
    controller1.strobe = value & 1;
    controller2.strobe = value & 1;

    if (controller1.strobe) {
        controller1.shift_reg = controller1.state;
        controller2.shift_reg = controller2.state;
    }
}

uint8_t Input::read_controller1() {
    uint8_t return_value = controller1.shift_reg & 1;
    if (!controller1.strobe) controller1.shift_reg >>= 1;
    return return_value | 0x40;
}

uint8_t Input::read_controller2() {
    uint8_t return_value = controller2.shift_reg & 1;
    if (!controller2.strobe) controller2.shift_reg >>= 1;
    return return_value | 0x40;
}

