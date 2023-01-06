#include <assert.h>
#include "ppu2C02.h"
#include "cpu6502.h"

PPU2C02state PPU_state;

int initPPU2C02(PPU2C02state *state) {
    state->scanline = 241;
    state->dot = 0;
    state->odd_frame = 0;
    state->nmi_occurred = 0;
    state->nmi_output = 0;

    state->oam.fill(0xff);

    return 1;
}

uint8_t PPUcycle(PPU2C02state *state, SDL_Surface *screenSurface) {

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
            PPU_state.VRAM_address &= 1055; //1055 = 0000010000011111b
            PPU_state.VRAM_address |= (PPU_state.t & ~1055 & ~(1<<15) );
        }
    }

    return 0;
}

uint8_t rendering_enabled() {
    return PPU_state.ppumask & ((1<<3)|(1<<4));
}

void handleVisibleScanline(PPU2C02state *state) {

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
    if( state->ppuctrl & (1 << 4) ) {
        pattern_base = 0x1000;
    }
    uint16_t pattern_index = state->vram[state->nametable_base];
    uint8_t row = ((state->VRAM_address&0x7000) >> 12);

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
                state->nametable_base = (0x2000 | (state->VRAM_address & 0x0FFF));
                break;
            case 3:
                fetchAttribute(state);
                break;
            case 5:
                state->bitmap_shift_0_latch = state->vram[pattern_base + pattern_index*16+row];
                break;
            case 7:
                state->bitmap_shift_1_latch = state->vram[pattern_base + pattern_index*16+row+8];
                break;
        }

    }

    if( state->dot == 256 ) {
        verinc();
    }

    if( state->dot == 257 ) {
        //v: ....F.. ...EDCBA = t: ....F.. ...EDCBA
        state->VRAM_address &= 31712; //31712 = 0111101111100000b
        state->VRAM_address |= (state->t & ~31712);
    }

}

void horinc() {
    if ((PPU_state.VRAM_address & 0x001F) == 0x001F) {
        PPU_state.VRAM_address &= ~0x001F;
        PPU_state.VRAM_address ^= 0x0400;
    }
    else {
        PPU_state.VRAM_address += 1;
    }
}

void verinc() {
    if ((PPU_state.VRAM_address & 0x7000) != 0x7000) {
        PPU_state.VRAM_address += 0x1000;
    }
    else {
        PPU_state.VRAM_address &= ~0x7000;
        uint16_t y = ((PPU_state.VRAM_address & 0x03E0) >> 5);
        if (y == 29) {
            y = 0;
            PPU_state.VRAM_address ^= 0x0800;
        }
        else if (y == 31) {
            y = 0;
        }
        else {
            y += 1;
        }
        PPU_state.VRAM_address = ((PPU_state.VRAM_address & ~0x03E0) | (y << 5));
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

    //OAMDMA
    else if(address == 0x4014) {
        int i;
        for(i=0; i<=0xFF; ++i) {
            writeSPRRAM(i, CPU_state.ram[(value << 8)|i] );
        }
        return 513 + PPU_state.odd_frame; //I think?
    }

    return 0;
}

void PPU2C02state::writeVRAM(uint16_t address, uint8_t value) {
    assert(address <= 0x3F20);
    this->vram[address] = value;
    if( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C ) {
        address -= 0x10;
    }
    address &= 0x3FFF;
    this->vram[address] = value;
}

uint8_t PPU2C02state::readVRAM(uint16_t address) {
    assert(address <= 0x3F20);
    if( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C ) {
        address -= 0x10;
    }
    address &= 0x3FFF;
    return this->vram[address];
}

void PPU2C02state::writeSPRRAM(uint8_t address, uint8_t value) {
    this->oam[address] = value;
}
uint8_t PPU2C02state::readSPRRAM(uint8_t address) {
    return this->oam[address];
}