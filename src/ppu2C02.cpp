#include <assert.h>
#include "ppu2C02.h"
#include "cpu6502.h"

PPU2C02state::PPU2C02state(SDL_Surface *screenSurface) {
    this->screenSurface = screenSurface;
    scanline = 241;
    dot = 0;
    odd_frame = 0;
    nmi_occurred = 0;
    nmi_output = 0;

    oam.fill(0xff);
}

void PPU2C02state::SetMapper(std::shared_ptr<Mapper> mapper) {
    this->mapper = mapper;
}

uint8_t PPU2C02state::PPUcycle() {

    if( nmi_output && nmi_occurred ) {
        nmi_occurred = 0;
        nmi = true;
    }

    //update cycles/scanlines
    dot += 1;
    if(dot == 341) {
        scanline += 1;
        dot = 0;
    }
    scanline %= 262;
    //skip first cycle on odd frames
    if(scanline == 0 && dot == 0 && odd_frame == 1 && rendering_enabled()) {
        dot = 1;
    }

    //visible scanlines
    if( scanline < 240 && rendering_enabled() ) {
        handleVisibleScanline();

        //visible cycles
        if( dot < 256 ) {
            renderPixel(screenSurface);
        }
    }

    //Post-screen scanline
    else if(scanline == 241) {
        if(dot == 1) {
            nmi_occurred = 1;
            odd_frame ^= 1;

            return 1;
        }
    }

    //Pre-screen scanline
    else if(scanline == 261) {

        handleVisibleScanline();

        if(dot == 2) {
            sprite_zero_hit = 0;
            nmi_occurred = 0;
        }

        else if(dot >= 280 && dot <= 304 && rendering_enabled()) {
            //v: IHGF.ED CBA..... = t: IHGF.ED CBA.....
            VRAM_address &= 1055; //1055 = 0000010000011111b
            VRAM_address |= (t & ~1055 & ~(1<<15) );
        }
    }

    return 0;
}

uint8_t PPU2C02state::rendering_enabled() {
    return ppumask & ((1<<3)|(1<<4));
}

void PPU2C02state::handleVisibleScanline() {

    if( dot == 0 ) {
        loadScanlineSprites();
        return;
    }
    if( !rendering_enabled() ) {
        return;
    }

    //shift the shift registers
    if( dot <= 255 || (dot > 320 && dot <= 336) ) {
        updatePPUrenderingData();
    }

    uint16_t pattern_base = 0x0000;
    if( ppuctrl & (1 << 4) ) {
        pattern_base = 0x1000;
    }
    uint16_t pattern_index = readVRAM(nametable_base);
    uint8_t row = ((VRAM_address&0x7000) >> 12);

    if( dot < 256 || (dot > 320 && dot <= 336) ) {

        //update the latches, depending on the cycle
        switch( dot % 8 ) {
            case 0:
                bitmap_shift_0 |= bitmap_shift_0_latch;
                bitmap_shift_1 |= bitmap_shift_1_latch;

                AT_shift_0 |= AT_shift_0_latch;
                AT_shift_1 |= AT_shift_1_latch;

                horinc();
                break;
            case 1:
                nametable_base = (0x2000 | (VRAM_address & 0x0FFF));
                break;
            case 3:
                fetchAttribute();
                break;
            case 5:
                bitmap_shift_0_latch = readVRAM(pattern_base + pattern_index*16+row);
                break;
            case 7:
                bitmap_shift_1_latch = readVRAM(pattern_base + pattern_index*16+row+8);
                break;
        }

    }

    if( dot == 256 ) {
        verinc();
    }

    if( dot == 257 ) {
        //v: ....F.. ...EDCBA = t: ....F.. ...EDCBA
        VRAM_address &= 31712; //31712 = 0111101111100000b
        VRAM_address |= (t & ~31712);
    }

}

void PPU2C02state::horinc() {
    if ((VRAM_address & 0x001F) == 0x001F) {
        VRAM_address &= ~0x001F;
        VRAM_address ^= 0x0400;
    }
    else {
        VRAM_address += 1;
    }
}

void PPU2C02state::verinc() {
    if ((VRAM_address & 0x7000) != 0x7000) {
        VRAM_address += 0x1000;
    }
    else {
        VRAM_address &= ~0x7000;
        uint16_t y = ((VRAM_address & 0x03E0) >> 5);
        if (y == 29) {
            y = 0;
            VRAM_address ^= 0x0800;
        }
        else if (y == 31) {
            y = 0;
        }
        else {
            y += 1;
        }
        VRAM_address = ((VRAM_address & ~0x03E0) | (y << 5));
    }
}

uint8_t PPU2C02state::readRegisters(uint16_t address) {

    uint8_t retVal = 0;

    //PPUSTATUS
    if(address == 0x2002) {
        w = 0;
        retVal = ((sprite_zero_hit << 6) | (nmi_occurred << 7));
        nmi_occurred = 0;
        if(scanline == 241 && dot == 1) {
            retVal = (sprite_zero_hit << 6);
        }
    }

    //OAMDATA
    else if(address == 0x2004) {
        return oam.at(OAM_address);
    }

    //PPUDATA
    else if(address == 0x2007) {
        //emulate 1 byte delay
        if( VRAM_address <= 0x3EFF ) {
            retVal = internal_buffer;
            internal_buffer = readVRAM( VRAM_address&0x3FFF );
        }
        else{
            internal_buffer = readVRAM( VRAM_address&0x3FFF );
            retVal = internal_buffer;
        }
        //increment address
        if( (ppuctrl & 4) == 0 ) {
            VRAM_address += 1;
        }
        else {
            VRAM_address += 32;
        }
    }

    return retVal;
}

uint8_t PPU2C02state::writeRegisters(uint16_t address, uint8_t value) {
    //PPUCTRL
    if(address == 0x2000) {
        ppuctrl = value;
        uint16_t val = value&3;
        this->t &= 0xF3FF;
        this->t |= (val << 10);
        this->nmi_output = ((value & (1<<7)) >> 7);
    }

    //PPUMASK
    else if(address == 0x2001) {
        ppumask = value;
    }

    //OAMDATA
    else if(address == 0x2004) {
        this->oam[this->OAM_address++] = value;
    }

    //PPUSCROLL
    else if(address == 0x2005) {
        if( this->w == 0 ) {
            this->t &= ~31; // 31 = 11111b
            this->t |= ((value&248) >> 3);
            this->x = (value & 7);
            //printf("setting coarse X to %d on scanline %d\n", ((value&248) >> 3), PPU_state.scanline);
        }
        else {
            //t: CBA..HG FED..... = d: HGFEDCBA
            this->t &= 3103; //3103 = 0000110000011111b
            uint16_t val = value;
            this->t |= ((val & 7) << 12); //CBA
            this->t |= ((val & 248) << 2); //HGFED
        }
        this->w ^= 1;
    }

    //PPUADDR
    else if(address == 0x2006) {
        if( this->w == 0 ) {
            this->t &= 0x00FF;
            uint16_t val = value;
            val <<= 8;
            this->t |= val;
        }
        else {
            this->t &= 0xFF00;
            this->t |= value;
            this->VRAM_address = this->t;
        }
        this->w ^= 1;
    }

    //PPUDATA
    else if(address == 0x2007) {
        writeVRAM( (VRAM_address&0x3FFF), value );
        if( (ppuctrl & 4) == 0 ) {
            VRAM_address += 1;
        }
        else {
            VRAM_address += 32;
        }
    }

    return 0;
}

void PPU2C02state::writeVRAM(uint16_t address, uint8_t value) {
    if(address <= 0x1FFF) {
        mapper->WriteChr(address, value);
        return;
    }
    assert(address <= 0x3F20);
    vram[address] = value;
    if( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C ) {
        address -= 0x10;
    }
    address &= 0x3FFF;
    vram[address] = value;
}

uint8_t PPU2C02state::readVRAM(uint16_t address) {
    assert(address <= 0x3F20);
    if( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C ) {
        address -= 0x10;
    }
    address &= 0x3FFF;
    switch(address) {
        case 0x0000 ... 0x1FFF:
            return mapper->ReadChr(address);
        default:
            return vram[address];
    }

}

void PPU2C02state::writeSPRRAM(uint8_t address, uint8_t value) {
    this->oam[address] = value;
}
uint8_t PPU2C02state::readSPRRAM(uint8_t address) {
    return this->oam[address];
}