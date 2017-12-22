#ifndef CONTROLLER_H_INCLUDED
#define CONTROLLER_H_INCLUDED

#include <SDL2/SDL.h>
#include <stdint.h>

struct Controller {
    uint8_t pointer;
    uint8_t button_status[8];
    uint8_t previous_write;
} NES_Controller;
typedef struct Controller Controller;

int initController(Controller *controller);

void handleInput(Controller *controller, SDL_Event *e);

void writeController(Controller *controller, uint8_t data);
uint8_t getNextButton(Controller *controller);

#endif // CONTROLLER_H_INCLUDED
