#include <assert.h>

#include "ppu2C02.h"


/******************
* rendering
******************/
 void setPixelColor(SDL_Surface* screenSurface, int x, int y, uint32_t color) {
    Uint32 *pixels = (Uint32 *)screenSurface->pixels;
    uint32_t dx = 0, dy=0;
    for(dx=0; dx<pixelWidth; ++dx) {
        for(dy=0; dy<pixelHeight; ++dy) {
            pixels[ ( ((y*pixelHeight)+dy) * screenSurface->w ) + (x*pixelWidth)+dx ] = color;
        }
    }
}

 void renderPixel(SDL_Surface* screenSurface, PPU2C02state *state) {
    //get bg color index
    uint8_t shift = 15-(PPU_state.x & 7);
    uint8_t bit_0 = (state->bitmap_shift_0 & (1 << shift)) >> shift;
    uint8_t bit_1 = (state->bitmap_shift_1 & (1 << shift)) >> shift;
    uint8_t bg_color_index = (bit_1 << 1) | bit_0;
    bit_0 = (state->AT_shift_0 & (1 << shift)) >> shift;
    bit_1 = (state->AT_shift_1 & (1 << shift)) >> shift;
    uint8_t bg_at_index = (bit_1 << 1) | bit_0;

    //get sprite color index and active sprite
    uint8_t sprite_color_index = 0;
    int active_sprite_index = getActiveSpriteIndex(state);
    if( active_sprite_index != -1 ) {
        bit_0 = (state->sprites[active_sprite_index].shift_register_0 & (1 << 7)) >> 7;
        bit_1 = (state->sprites[active_sprite_index].shift_register_1 & (1 << 7)) >> 7;
        sprite_color_index = (bit_1 << 1) | bit_0;
    }

    //draw the pixel on the screen, depending on color and priority
    if( bg_color_index == 0 && sprite_color_index == 0 ) {
        setPixelColor(screenSurface, state->dot, state->scanline, ppu_colors[state->vram[0x3F00]]);
    }
    else if( (sprite_color_index != 0 && bg_color_index == 0) ||
             (sprite_color_index != 0 && bg_color_index != 0 && (state->sprites[active_sprite_index].byte2 & (1<<5)) == 0) ) {
        assert( active_sprite_index != -1 );
        uint16_t palette_base = getSpritePaletteBase(state->sprites[active_sprite_index].attribute);
        uint8_t color_value = PPU_state.readVRAM(palette_base + sprite_color_index);
        uint32_t color = ppu_colors[color_value];
        setPixelColor(screenSurface, state->dot, state->scanline, color);
    }
    else {
        uint16_t palette_base = getBackgroundPaletteBase(bg_at_index);
        uint8_t color_value = PPU_state.readVRAM(palette_base + bg_color_index);
        uint32_t color = ppu_colors[color_value];
        setPixelColor(screenSurface, state->dot, state->scanline, color);
    }

    //handle sprite zero hit
    if( active_sprite_index != -1 && state->sprites[active_sprite_index].sprite_index == 0 && sprite_color_index != 0 && bg_color_index == 0 ) {
        state->sprite_zero_hit = 1;
    }
}

/******************
* loading
******************/

 void loadScanlineSprites(PPU2C02state *state) {
    state->num_sprites = 0;
    int i=0;
    for(i=0; i<8; ++i) {
        state->sprites[i].shift_register_0 = 0;
        state->sprites[i].shift_register_1 = 0;
    }

    for(i=0x00; i<0xFF; i+=4) {

        uint8_t y = state->readSPRRAM(i+0)+1;
        uint8_t pattern_index = state->readSPRRAM(i+1);
        uint8_t byte2 = state->readSPRRAM(i+2);
        uint8_t x = state->readSPRRAM(i+3);

        if( y <= state->scanline && y+8 > state->scanline ) {

            uint16_t pattern_base = 0x0000;
            if( (PPU_state.ppuctrl & (1 << 3)) ) {
                pattern_base = 0x1000;
            }

            int row = state->scanline-y;
            uint8_t pattern_0 = state->vram[pattern_base + (pattern_index*16+row)];
            uint8_t pattern_1 = state->vram[pattern_base + (pattern_index*16+row+8)];

            //flip y
            if( byte2 & (1 << 7) ) {
                pattern_0 = state->vram[pattern_base + (pattern_index*16+(7-row))];
                pattern_1 = state->vram[pattern_base + (pattern_index*16+(7-row)+8)];
            }

            //flip x, reverse the bits in the patterns
            if(byte2 & (1 << 6) ) {
                uint8_t new_pattern_0 = 0;
                uint8_t new_pattern_1 = 0;
                int bit;
                for(bit=0; bit<8; ++bit) {
                    new_pattern_0 <<= 1;
                    new_pattern_0 |= (pattern_0&1);
                    pattern_0 >>= 1;
                    new_pattern_1 <<= 1;
                    new_pattern_1 |= (pattern_1&1);
                    pattern_1 >>= 1;
                }
                pattern_0 = new_pattern_0;
                pattern_1 = new_pattern_1;
            }

            state->sprites[state->num_sprites].sprite_index = i;
            state->sprites[state->num_sprites].x = x;
            state->sprites[state->num_sprites].attribute = (byte2 & 3);
            state->sprites[state->num_sprites].shift_register_0 = pattern_0;
            state->sprites[state->num_sprites].shift_register_1 = pattern_1;
            state->sprites[state->num_sprites].shifts_remaining = 8;
            state->sprites[state->num_sprites].byte2 = byte2;

            state->num_sprites += 1;
            if(state->num_sprites == 8) {
                break;
            }
        }
    }

}

 int getActiveSpriteIndex(PPU2C02state *state) {
    uint8_t i;
    for(i=0; i<state->num_sprites; ++i) {
        if( state->sprites[i].x == 0 && state->sprites[i].shifts_remaining > 0 ) {
            uint8_t bit_0 = (state->sprites[i].shift_register_0 & (1 << 7)) >> 7;
            uint8_t bit_1 = (state->sprites[i].shift_register_1 & (1 << 7)) >> 7;
            uint8_t bg_color_index = (bit_1 << 1) | bit_0;
            if( bg_color_index > 0 ) {
                return i;
            }
        }
    }
    return -1;
}

 void updatePPUrenderingData(PPU2C02state *state) {
    state->bitmap_shift_0 <<= 1;
    state->bitmap_shift_1 <<= 1;
    state->bitmap_shift_0 &= ~1;
    state->bitmap_shift_1 &= ~1;

    state->AT_shift_0 <<= 1;
    state->AT_shift_1 <<= 1;
    state->AT_shift_0 &= ~1;
    state->AT_shift_1 &= ~1;

    int i;
    for(i=0; i<state->num_sprites; ++i) {
        if( state->sprites[i].x == 0 && state->sprites[i].shifts_remaining > 0) {
            state->sprites[i].shift_register_0 <<= 1;
            state->sprites[i].shift_register_1 <<= 1;
            state->sprites[i].shifts_remaining -= 1;
        }
        else {
            state->sprites[i].x -= 1;
        }
    }
}

/******************
* fetching values
******************/

 uint16_t getSpritePaletteBase(uint8_t attribute_value) {
    switch(attribute_value) {
        case 0:
            return 0x3F10;
        case 1:
            return 0x3F14;
        case 2:
            return 0x3F18;
        case 3:
            return 0x3F1C;
    }
    return 0x3F10;
}

 uint16_t getBackgroundPaletteBase(uint16_t attribute_value) {
    switch(attribute_value) {
        case 0:
            return 0x3F00;
        case 1:
            return 0x3F04;
        case 2:
            return 0x3F08;
        case 3:
            return 0x3F0C;
    }
    return 0x3F00;
}

 void fetchAttribute(PPU2C02state *state) {
    uint16_t attribute_address = (0x23C0 | (state->VRAM_address & 0x0C00) | ((state->VRAM_address >> 4) & 0x38) | ((state->VRAM_address >> 2) & 0x07));
    uint8_t at = getAttributeTableValue(attribute_address, (state->VRAM_address & 0x001F)*8, ((state->VRAM_address & (0x001F << 5)) >> 5)*8);
    if(at & 1) {
        state->AT_shift_0_latch = 0xFF;
    }
    else {
        state->AT_shift_0_latch = 0x00;
    }
    if(at & 2) {
        state->AT_shift_1_latch = 0xFF;
    }
    else {
        state->AT_shift_1_latch = 0x00;
    }
}

 uint8_t getAttributeTableValue(uint16_t attribute_address, uint8_t x, uint8_t y) {
    uint8_t attribute_value = PPU_state.readVRAM(attribute_address);

    uint8_t bottom = 1;
    uint8_t right = 1;
    if( (y/32)*32 == (y/16)*16 ) {
        //top
        bottom = 0;
    }
    if( (x/32)*32 == (x/16)*16 ) {
        //left
        right = 0;
    }

    uint8_t mask = 0;
    if( bottom == 0 && right == 0 ) {
        mask = (1 << 1) | (1 << 0);
        attribute_value = (attribute_value & mask);
    }
    if( bottom == 0 && right == 1 ) {
        mask = (1 << 3) | (1 << 2);
        attribute_value = (attribute_value & mask) >> 2;
    }
    else if( bottom == 1 && right == 0 ) {
        mask = (1 << 5) | (1 << 4);
        attribute_value = (attribute_value & mask) >> 4;
    }
    else if( bottom == 1 && right == 1 ) {
        mask = (1 << 7) | (1 << 6);
        attribute_value = (attribute_value & mask) >> 6;
    }

    return attribute_value;
}
