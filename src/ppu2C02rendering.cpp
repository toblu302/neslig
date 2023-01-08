#include <assert.h>

#include "ppu2C02.h"


/******************
* rendering
******************/
 void PPU2C02state::setPixelColor(SDL_Surface* screenSurface, int x, int y, uint32_t color) {
    Uint32 *pixels = (Uint32 *)screenSurface->pixels;
    uint32_t dx = 0, dy=0;
    for(dx=0; dx<pixelWidth; ++dx) {
        for(dy=0; dy<pixelHeight; ++dy) {
            pixels[ ( ((y*pixelHeight)+dy) * screenSurface->w ) + (x*pixelWidth)+dx ] = color;
        }
    }
}

void PPU2C02state::renderPixel(SDL_Surface* screenSurface) {
    //get bg color index
    uint8_t shift = 15-(x & 7);
    uint8_t bit_0 = (bitmap_shift_0 & (1 << shift)) >> shift;
    uint8_t bit_1 = (bitmap_shift_1 & (1 << shift)) >> shift;
    uint8_t bg_color_index = (bit_1 << 1) | bit_0;
    bit_0 = (AT_shift_0 & (1 << shift)) >> shift;
    bit_1 = (AT_shift_1 & (1 << shift)) >> shift;
    uint8_t bg_at_index = (bit_1 << 1) | bit_0;

    //get sprite color index and active sprite
    uint8_t sprite_color_index = 0;
    int active_sprite_index = getActiveSpriteIndex();
    if( active_sprite_index != -1 ) {
        bit_0 = (sprites[active_sprite_index].shift_register_0 & (1 << 7)) >> 7;
        bit_1 = (sprites[active_sprite_index].shift_register_1 & (1 << 7)) >> 7;
        sprite_color_index = (bit_1 << 1) | bit_0;
    }

    //draw the pixel on the screen, depending on color and priority
    if( bg_color_index == 0 && sprite_color_index == 0 ) {
        setPixelColor(screenSurface, dot, scanline, ppu_colors[readVRAM(0x3F00)]);
    }
    else if( (sprite_color_index != 0 && bg_color_index == 0) ||
             (sprite_color_index != 0 && bg_color_index != 0 && (sprites[active_sprite_index].byte2 & (1<<5)) == 0) ) {
        assert( active_sprite_index != -1 );
        uint16_t palette_base = getSpritePaletteBase(sprites[active_sprite_index].attribute);
        uint8_t color_value = readVRAM(palette_base + sprite_color_index);
        uint32_t color = ppu_colors[color_value];
        setPixelColor(screenSurface, dot, scanline, color);
    }
    else {
        uint16_t palette_base = getBackgroundPaletteBase(bg_at_index);
        uint8_t color_value = readVRAM(palette_base + bg_color_index);
        uint32_t color = ppu_colors[color_value];
        setPixelColor(screenSurface, dot, scanline, color);
    }

    //handle sprite zero hit
    if( active_sprite_index != -1 && sprites[active_sprite_index].sprite_index == 0 && sprite_color_index != 0 && bg_color_index == 0 ) {
        sprite_zero_hit = 1;
    }
}

/******************
* loading
******************/

 void PPU2C02state::loadScanlineSprites() {
    num_sprites = 0;
    int i=0;
    for(i=0; i<8; ++i) {
        sprites[i].shift_register_0 = 0;
        sprites[i].shift_register_1 = 0;
    }

    for(i=0x00; i<0xFF; i+=4) {

        uint8_t y = readSPRRAM(i+0)+1;
        uint8_t pattern_index = readSPRRAM(i+1);
        uint8_t byte2 = readSPRRAM(i+2);
        uint8_t x = readSPRRAM(i+3);

        if( y <= scanline && y+8 > scanline ) {

            uint16_t pattern_base = 0x0000;
            if( (ppuctrl & (1 << 3)) ) {
                pattern_base = 0x1000;
            }

            int row = scanline-y;
            uint8_t pattern_0 = readVRAM(pattern_base + (pattern_index*16+row));
            uint8_t pattern_1 = readVRAM(pattern_base + (pattern_index*16+row+8));

            //flip y
            if( byte2 & (1 << 7) ) {
                pattern_0 = readVRAM(pattern_base + (pattern_index*16+(7-row)));
                pattern_1 = readVRAM(pattern_base + (pattern_index*16+(7-row)+8));
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

            sprites[num_sprites].sprite_index = i;
            sprites[num_sprites].x = x;
            sprites[num_sprites].attribute = (byte2 & 3);
            sprites[num_sprites].shift_register_0 = pattern_0;
            sprites[num_sprites].shift_register_1 = pattern_1;
            sprites[num_sprites].shifts_remaining = 8;
            sprites[num_sprites].byte2 = byte2;

            num_sprites += 1;
            if(num_sprites == 8) {
                break;
            }
        }
    }

}

 int PPU2C02state::getActiveSpriteIndex() {
    uint8_t i;
    for(i=0; i<num_sprites; ++i) {
        if( sprites[i].x == 0 && sprites[i].shifts_remaining > 0 ) {
            uint8_t bit_0 = (sprites[i].shift_register_0 & (1 << 7)) >> 7;
            uint8_t bit_1 = (sprites[i].shift_register_1 & (1 << 7)) >> 7;
            uint8_t bg_color_index = (bit_1 << 1) | bit_0;
            if( bg_color_index > 0 ) {
                return i;
            }
        }
    }
    return -1;
}

 void PPU2C02state::updatePPUrenderingData() {
    bitmap_shift_0 <<= 1;
    bitmap_shift_1 <<= 1;
    bitmap_shift_0 &= ~1;
    bitmap_shift_1 &= ~1;

    AT_shift_0 <<= 1;
    AT_shift_1 <<= 1;
    AT_shift_0 &= ~1;
    AT_shift_1 &= ~1;

    int i;
    for(i=0; i<num_sprites; ++i) {
        if( sprites[i].x == 0 && sprites[i].shifts_remaining > 0) {
            sprites[i].shift_register_0 <<= 1;
            sprites[i].shift_register_1 <<= 1;
            sprites[i].shifts_remaining -= 1;
        }
        else {
            sprites[i].x -= 1;
        }
    }
}

/******************
* fetching values
******************/

 uint16_t PPU2C02state::getSpritePaletteBase(uint8_t attribute_value) {
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

 uint16_t PPU2C02state::getBackgroundPaletteBase(uint16_t attribute_value) {
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

 void PPU2C02state::fetchAttribute() {
    uint16_t attribute_address = (0x23C0 | (VRAM_address & 0x0C00) | ((VRAM_address >> 4) & 0x38) | ((VRAM_address >> 2) & 0x07));
    uint8_t at = getAttributeTableValue(attribute_address, (VRAM_address & 0x001F)*8, ((VRAM_address & (0x001F << 5)) >> 5)*8);
    if(at & 1) {
        AT_shift_0_latch = 0xFF;
    }
    else {
        AT_shift_0_latch = 0x00;
    }
    if(at & 2) {
        AT_shift_1_latch = 0xFF;
    }
    else {
        AT_shift_1_latch = 0x00;
    }
}

 uint8_t PPU2C02state::getAttributeTableValue(uint16_t attribute_address, uint8_t x, uint8_t y) {
    uint8_t attribute_value = readVRAM(attribute_address);

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
