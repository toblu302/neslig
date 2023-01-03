#include "controller.h"
#include "MMU.h"

Controller NES_Controller;

int initController(Controller *controller) {
    controller->pointer = 0;
    int i;
    for(i=0; i<8; ++i) {
        controller->button_status[i] = 0;
    }
    controller->previous_write = 0;

    return 0;
}

void handleInput(Controller *controller, SDL_Event *e) {
    if( e->type == SDL_KEYDOWN )
    {
        switch( e->key.keysym.sym ) {
            case SDLK_z:
                controller->button_status[0] = 1; //A
                break;
            case SDLK_x:
                controller->button_status[1] = 1; //B
                break;
            case SDLK_SPACE:
                controller->button_status[2] = 1; //SELECT
                break;
            case SDLK_RETURN:
                controller->button_status[3] = 1; //START
                break;
            case SDLK_UP:
                controller->button_status[4] = 1; //UP
                break;
            case SDLK_DOWN:
                controller->button_status[5] = 1; //DOWN
                break;
            case SDLK_LEFT:
                controller->button_status[6] = 1; //LEFT
                break;
            case SDLK_RIGHT:
                controller->button_status[7] = 1; //RIGHT
                break;
            default:
                break;
        }
    }
    else if( e->type == SDL_KEYUP )
    {
        switch( e->key.keysym.sym ) {
            case SDLK_z:
                controller->button_status[0] = 0; //A
                break;
            case SDLK_x:
                controller->button_status[1] = 0; //B
                break;
            case SDLK_SPACE:
                controller->button_status[2] = 0; //SELECT
                break;
            case SDLK_RETURN:
                controller->button_status[3] = 0; //START
                break;
            case SDLK_UP:
                controller->button_status[4] = 0; //UP
                break;
            case SDLK_DOWN:
                controller->button_status[5] = 0; //DOWN
                break;
            case SDLK_LEFT:
                controller->button_status[6] = 0; //LEFT
                break;
            case SDLK_RIGHT:
                controller->button_status[7] = 0; //RIGHT
                break;
            default:
                break;
        }
    }
}

void writeController(Controller *controller, uint8_t data) {
    if( controller->previous_write == 1 && data == 0) {
        controller->pointer = 0;
    }
    controller->previous_write = data;
}

uint8_t getNextButton(Controller *controller) {
    uint8_t retval = controller->button_status[ controller->pointer++ ];
    controller->pointer = controller->pointer % 8;
    return retval;
}
