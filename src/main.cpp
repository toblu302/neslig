#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <assert.h>

#include "controller.h"
#include "MMU.h"
#include "cpu6502.h"
#include "ppu2C02.h"

int main(int argc, char *argv[])
{
    if( argc < 2 ) {
        printf("Error: No .nes-file supplied\n");
        return 1;
    }
    //open a handle to the file
    FILE *fp;
    fp = fopen(argv[1], "r");

    if(!fp) {
        printf("Error: Could not load the file.\n");
        return 1;
    }

    //find out the size of the file
    fseek(fp, 0, SEEK_END);
    int string_size = ftell(fp);
    rewind(fp);

    //read the entire file into a buffer
    unsigned char *cartridgebuffer = (unsigned char*) malloc(sizeof(unsigned char) * (string_size + 1) );
    int read_size = fread(cartridgebuffer, sizeof(unsigned char), string_size, fp);
    cartridgebuffer[string_size] = '\0';

    //if there was some reading error, abort
    if(read_size != string_size) {
        printf("Error: Something went wrong in reading the file.\n");
        free(cartridgebuffer);
        fclose(fp);
        return 1;
    }

    fclose(fp);

    //if the file is not an iNES-file, abort
    if(cartridgebuffer[0] != 'N' || cartridgebuffer[1] != 'E' || cartridgebuffer[2] != 'S' || cartridgebuffer[3] != 0x1a) {
        printf("Error: File is not an iNES-file.\n");
    }

    //********************
    //program starts here
    //********************
    int numPRGROM = cartridgebuffer[4];
    int numCHRROM = cartridgebuffer[5];
    int controlByte1 = cartridgebuffer[6];
    int controlByte2 = cartridgebuffer[7];
    int numRAM = cartridgebuffer[8];

    int trainer = (controlByte1 & (1 << 2));

    int mapper = ( (controlByte2 & 0xF0) | ((controlByte1 & 0xF0) >> 4));

    printf("PRGROM: %d  CHRROM: %d  numRAM: %d Trainer: %d  mapper: %d\n", numPRGROM, numCHRROM, numRAM, trainer, mapper);
    assert(mapper == 0);

    initMMU();

    //Copy the ROM into the CPU's memory
    memcpy(MMU.RAM+0x8000, cartridgebuffer+0x10, numPRGROM*0x4000);
    if(numPRGROM == 1) {
        memcpy(MMU.RAM+0xC000, cartridgebuffer+0x10, numPRGROM*0x4000);
    }

    //Copy the ROM into the PPU's memory
    memcpy(MMU.VRAM, cartridgebuffer+0x10+0x4000*numPRGROM, 0x2000*numCHRROM);

    //initalize the CPU
    initCPU6502(&CPU_state);

    //initalize the PPU
    initPPU2C02(&PPU_state);

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

    free(cartridgebuffer);
    return 0;
}
