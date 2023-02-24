#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <assert.h>
#include <memory>

#include "controller.h"
#include "cpu6502.h"
#include "ppu2C02.h"
#include "filereader.h"

int main(int argc, char *argv[])
{
    if( argc < 2 ) {
        printf("Error: No .nes-file supplied\n");
        return 1;
    }

    std::shared_ptr<Mapper> mapper = read_file(argv[1]);
    std::cout << *mapper << std::endl;


    //initalize the Controller
    initController(&NES_Controller);

    //Initalize SDL
    SDL_Window* window = NULL;
    SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO );
    window = SDL_CreateWindow( "NESlig", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256*pixelWidth, 240*pixelHeight, SDL_WINDOW_SHOWN );
	SDL_Surface* screenSurface = SDL_GetWindowSurface( window );
    //SDL_GL_SetSwapInterval(0);

    PPU2C02state ppu(screenSurface);
    CPU6502state cpu(&ppu, mapper);

    //main loop
    SDL_Event e;
    int quit = 0;
    int paused = 0;
    uint32_t frame_count = 0;
    uint32_t frame_start = SDL_GetTicks();
    double delay = 1000.0/60.1;
    while(!quit) {

        //Handle input
        while( SDL_PollEvent( &e ) != 0 ) {
            if( e.type == SDL_QUIT ) {
                quit = 1;
            }
            if( e.type == SDL_KEYDOWN ) {
                if( e.key.keysym.sym == SDLK_PAUSE ) {
                    paused ^= 1;
                }
            }
            handleInput(&NES_Controller, &e);
        }


        // Emulate CPU
        // The CPU will clock both the PPU and the APU
        uint32_t frame_start = SDL_GetTicks();

        uint current_frame = ppu.GetCurrentFrame();
        while(current_frame == ppu.GetCurrentFrame()) {

            // If there are enough audio samples, simply wait until the
            // audio callback clears some of them.
            if(cpu.apu.generated_samples < cpu.apu.samples_per_callback) {
                cpu.fetchAndExecute();
            }
        }

        //printf("Frame %d rendered!\n", frame_count);
        uint32_t frame_time = SDL_GetTicks() - frame_start;
        if(frame_time < delay) {
            SDL_Delay(delay-frame_time);
        }

        SDL_UpdateWindowSurface(window);

    }

    return 0;
}
