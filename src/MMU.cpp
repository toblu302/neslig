#include "MMU.h"
#include <assert.h>

#include "controller.h"
#include "ppu2C02.h"

MemoryManager MMU;

uint8_t writeRAM(uint16_t address, uint8_t value) {

    while(address >= 0x2008 && address < 0x4000) {
        address -= 8;
    }
    while(address >= 0x0800 && address < 0x2000) {
        address -= 0x0800;
    }

    if( (address >= 0x2000 && address <= 0x2007 && value != 0) || address == 0x4014 ) {
        if( address == 0x2003 ) {
            assert(1 == 0);
        }
    }

    //PPUMASK
    if(address == 0x2000) {
        uint16_t val = value&3;
        MMU.t &= 0xF3FF;
        MMU.t |= (val << 10);
        PPU_state.nmi_output = ((value & (1<<7)) >> 7);
    }

    //OAMDATA
    else if(address == 0x2004) {
        MMU.OAM[MMU.OAM_address++] = value;
    }

    //PPUSCROLL
    else if(address == 0x2005) {
        if( MMU.w == 0 ) {
            MMU.t &= ~31; // 31 = 11111b
            MMU.t |= ((value&248) >> 3);
            MMU.x = (value & 7);
            //printf("setting coarse X to %d on scanline %d\n", ((value&248) >> 3), PPU_state.scanline);
        }
        else {
            //t: CBA..HG FED..... = d: HGFEDCBA
            MMU.t &= 3103; //3103 = 0000110000011111b
            uint16_t val = value;
            MMU.t |= ((val & 7) << 12); //CBA
            MMU.t |= ((val & 248) << 2); //HGFED
        }
        MMU.w ^= 1;
    }

    //PPUADDR
    else if(address == 0x2006) {
        if( MMU.w == 0 ) {
            MMU.t &= 0x00FF;
            uint16_t val = value;
            val <<= 8;
            MMU.t |= val;
        }
        else {
            MMU.t &= 0xFF00;
            MMU.t |= value;
            MMU.VRAM_address = MMU.t;
        }
        MMU.w ^= 1;
        //MMU.VRAM_address = ((MMU.VRAM_address << 8) | value);
    }

    //PPUDATA
    else if(address == 0x2007) {
        writeVRAM( (MMU.VRAM_address&0x3FFF), value );
        if( (MMU.RAM[0x2000] & 4) == 0 ) {
            MMU.VRAM_address += 1;
        }
        else {
            MMU.VRAM_address += 32;
        }
    }

    //OAMDMA
    else if(address == 0x4014) {
        int i;
        for(i=0; i<=0xFF; ++i) {
            writeSPRRAM(i, MMU.RAM[(value << 8)|i] );
        }
        return 513 + PPU_state.odd_frame; //I think?
    }

    //Controller 1
    else if(address == 0x4016) {
        writeController(&NES_Controller, value);
    }

    MMU.RAM[address] = value;
    return 0;
}

uint8_t readRAM(uint16_t address) {
    while(address >= 0x2008 && address < 0x4000) {
        address -= 8; //handle mirroring
    }
    while(address >= 0x0800 && address < 0x2000) {
        address -= 0x0800; //handle mirroring
    }

    uint8_t retVal = MMU.RAM[address];

    //PPUSTATUS
    if(address == 0x2002) {
        MMU.w = 0;
        retVal = ((PPU_state.sprite_zero_hit << 6) | (PPU_state.nmi_occurred << 7));
        PPU_state.nmi_occurred = 0;
        if(PPU_state.scanline == 241 && PPU_state.dot == 1) {
            retVal = (PPU_state.sprite_zero_hit << 6);
        }
    }

    //OAMDATA
    if(address == 0x2004) {
        return MMU.OAM[MMU.OAM_address];
    }

    //PPUDATA
    else if(address == 0x2007) {
        //emulate 1 byte delay
        if( MMU.VRAM_address <= 0x3EFF ) {
            retVal = MMU.internal_buffer;
            MMU.internal_buffer = readVRAM( MMU.VRAM_address&0x3FFF );
        }
        else{
            MMU.internal_buffer = readVRAM( MMU.VRAM_address&0x3FFF );
            retVal = MMU.internal_buffer;
        }
        //increment address
        if( (MMU.RAM[0x2000] & 4) == 0 ) {
            MMU.VRAM_address += 1;
        }
        else {
            MMU.VRAM_address += 32;
        }
    }

    //Controller 1
    else if(address == 0x4016) {
        retVal = getNextButton(&NES_Controller);
    }

    return retVal;
}

void writeVRAM(uint16_t address, uint8_t value) {
    assert(address <= 0x3F20);
    MMU.VRAM[address] = value;
    if( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C ) {
        address -= 0x10;
    }
    address &= 0x3FFF;
    MMU.VRAM[address] = value;
}

uint8_t readVRAM(uint16_t address) {
    assert(address <= 0x3F20);
    if( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C ) {
        address -= 0x10;
    }
    address &= 0x3FFF;
    return MMU.VRAM[address];
}


void writeSPRRAM(uint8_t address, uint8_t value) {
    MMU.OAM[address] = value;
}
uint8_t readSPRRAM(uint8_t address) {
    return MMU.OAM[address];
}

void dumpRAM() {
    uint16_t address = 0;
    uint16_t count = 0;
    printf("\nDump of RAM:\n");
    for( address=0x3F00; address<=0x3F20; ++address) {
            if( count % 16 == 0 ) {
                printf("\n%X: ", address);
            }
            printf("%X, ", MMU.VRAM[address]);
            count += 1;
    }
    printf("\n");
}

void dumpSPRRAM() {
    uint16_t address = 0;
    uint16_t count = 0;
    printf("Dump of SPRRAM:\n");
    for( address=0; address<=0xFF; ++address) {
            if( count % 16 == 0 ) {
                printf("\n%X: ", address);
            }
            printf("%X, ", MMU.OAM[address]);
            count += 1;
    }
    printf("\n");
}

void dumpVRAM() {
    printf("Dump of interesting VRAM:\n");
    uint16_t count = 0;
    uint16_t address = 0;
    for( address=0x2000; address<0x23FF; ++address) {
        if( count % 16 == 0 ) {
            printf("\n%X: ", address);
        }
        printf("%X, ", MMU.VRAM[address]);
        count += 1;
    }
}
