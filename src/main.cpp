#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <assert.h>

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

    Cartridge cartridge = read_file(argv[1]);

    printf("PRGROM: %d  CHRROM: %d  mapper: %d\n", cartridge.prg_rom_banks.size(), cartridge.chr_rom_banks.size(), cartridge.mapper);
    //assert(cartridge.mapper == 0);

    //initalize the CPU
    initCPU6502(&CPU_state, cartridge);

    //initalize the PPU
    initPPU2C02(&PPU_state);

    //Copy the ROM into the PPU's memory
    std::copy(cartridge.chr_rom_banks[0].begin(), cartridge.chr_rom_banks[0].end(), (&PPU_state)->vram.begin());

    //initalize the Controller
    initController(&NES_Controller);

    //Initalize SDL
    SDL_Window* window = NULL;
    SDL_Init( SDL_INIT_VIDEO );
    window = SDL_CreateWindow( "NESlig", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256*pixelWidth, 240*pixelHeight, SDL_WINDOW_SHOWN );
	SDL_Surface* screenSurface = SDL_GetWindowSurface( window );


    //main loop
    SDL_Event e;
    int quit = 0;
    int paused = 0;
    uint32_t frame_count = 0;
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

        //emulate CPU and PPU
        uint8_t frame_finished = 0;
        if( paused == 0 ) {
            uint8_t cycles = fetchAndExecute(&CPU_state);

            uint8_t loop = cycles*3;
            while( loop != 0 )
            {
                frame_finished |= PPUcycle(&PPU_state, screenSurface);
                loop -= 1;
            }
        }

        if( frame_finished ) {
            frame_count += 1;
            //printf("Frame %d rendered!\n", frame_count);
            SDL_UpdateWindowSurface(window);
        }

    }

    return 0;
}
