#include <assert.h>
#include "MMU.h"
#include "ppu2C02.h"
#include "cpu6502.h"

int initPPU2C02(PPU2C02state *state) {
    state->scanline = 241;
    state->dot = 0;
    state->odd_frame = 0;
    state->nmi_occurred = 0;
    state->nmi_output = 0;

    return 1;
}

inline uint8_t PPUcycle(PPU2C02state *state, SDL_Surface *screenSurface) {

    if( state->nmi_output && state->nmi_occurred ) {
        state->nmi_occurred = 0;
        NMI(&CPU_state);
    }

    //update cycles/scanlines
    state->dot += 1;
    if(state->dot == 341) {
        state->scanline += 1;
        state->dot = 0;
    }
    state->scanline %= 262;
    //skip first cycle on odd frames
    if(state->scanline == 0 && state->dot == 0 && state->odd_frame == 1 && rendering_enabled()) {
        state->dot = 1;
    }

    //visible scanlines
    if( state->scanline < 240 && rendering_enabled() ) {
        handleVisibleScanline(state);

        //visible cycles
        if( state->dot < 256 ) {
            renderPixel(screenSurface, state);
        }
    }

    //Post-screen scanline
    else if(state->scanline == 241) {
        if(state->dot == 1) {
            state->nmi_occurred = 1;
            state->odd_frame ^= 1;

            return 1;
        }
    }

    //Pre-screen scanline
    else if(state->scanline == 261) {

        handleVisibleScanline(state);

        if(state->dot == 2) {
            state->sprite_zero_hit = 0;
            state->nmi_occurred = 0;
        }

        else if(state->dot >= 280 && state->dot <= 304 && rendering_enabled()) {
            //v: IHGF.ED CBA..... = t: IHGF.ED CBA.....
            MMU.VRAM_address &= 1055; //1055 = 0000010000011111b
            MMU.VRAM_address |= (MMU.t & ~1055 & ~(1<<15) );
        }
    }

    return 0;
}

inline uint8_t rendering_enabled() {
    return MMU.RAM[0x2001] & ((1<<3)|(1<<4));
}

inline void handleVisibleScanline(PPU2C02state *state) {

    if( state->dot == 0 ) {
        loadScanlineSprites(state);
        return;
    }
    if( !rendering_enabled() ) {
        return;
    }

    //shift the shift registers
    if( state->dot <= 255 || (state->dot > 320 && state->dot <= 336) ) {
        updatePPUrenderingData(state);
    }

    uint16_t pattern_base = 0x0000;
    if( MMU.RAM[0x2000] & (1 << 4) ) {
        pattern_base = 0x1000;
    }
    uint16_t pattern_index = MMU.VRAM[state->nametable_base];
    uint8_t row = ((MMU.VRAM_address&0x7000) >> 12);

    if( state->dot < 256 || (state->dot > 320 && state->dot <= 336) ) {

        //update the latches, depending on the cycle
        switch( state->dot % 8 ) {
            case 0:
                state->bitmap_shift_0 |= state->bitmap_shift_0_latch;
                state->bitmap_shift_1 |= state->bitmap_shift_1_latch;

                state->AT_shift_0 |= state->AT_shift_0_latch;
                state->AT_shift_1 |= state->AT_shift_1_latch;

                horinc();
                break;
            case 1:
                state->nametable_base = (0x2000 | (MMU.VRAM_address & 0x0FFF));
                break;
            case 3:
                fetchAttribute(state);
                break;
            case 5:
                state->bitmap_shift_0_latch = MMU.VRAM[pattern_base + pattern_index*16+row];
                break;
            case 7:
                state->bitmap_shift_1_latch = MMU.VRAM[pattern_base + pattern_index*16+row+8];
                break;
        }

    }

    if( state->dot == 256 ) {
        verinc();
    }

    if( state->dot == 257 ) {
        //v: ....F.. ...EDCBA = t: ....F.. ...EDCBA
        MMU.VRAM_address &= 31712; //31712 = 0111101111100000b
        MMU.VRAM_address |= (MMU.t & ~31712);
    }

}

inline void horinc() {
    if ((MMU.VRAM_address & 0x001F) == 0x001F) {
        MMU.VRAM_address &= ~0x001F;
        MMU.VRAM_address ^= 0x0400;
    }
    else {
        MMU.VRAM_address += 1;
    }
}

inline void verinc() {
    if ((MMU.VRAM_address & 0x7000) != 0x7000) {
        MMU.VRAM_address += 0x1000;
    }
    else {
        MMU.VRAM_address &= ~0x7000;
        uint16_t y = ((MMU.VRAM_address & 0x03E0) >> 5);
        if (y == 29) {
            y = 0;
            MMU.VRAM_address ^= 0x0800;
        }
        else if (y == 31) {
            y = 0;
        }
        else {
            y += 1;
        }
        MMU.VRAM_address = ((MMU.VRAM_address & ~0x03E0) | (y << 5));
    }
}
