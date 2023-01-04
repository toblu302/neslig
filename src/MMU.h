#ifndef MMU_H_INCLUDED
#define MMU_H_INCLUDED

#define PPUCTRL 0x2000
#define PPUMASK 0x2001
#define PPUSTATUS 0x2002
#define OAMADDR 0x2003
#define OAMDATA 0x2004
#define PPUSCROLL 0x2005
#define PPUADDR 0x2006
#define PPUDATA 0x2007
#define OAMDMA 0x4014

#include <stdint.h>
#include <array>

class MemoryManager {
    public:
        MemoryManager() {
            OAM.fill(0xff);
        }

        std::array<uint8_t, 0x10000> RAM;
        std::array<uint8_t, 0x10000> VRAM;
        std::array<uint8_t, 0x100> OAM;

        uint8_t w = 0;
        uint16_t t = 0;
        uint16_t x = 0;
        uint16_t y = 0;

        uint16_t VRAM_address = 0;
        uint8_t OAM_address = 0;
        uint8_t internal_buffer = 0;
};

extern MemoryManager MMU;

uint8_t writeRAM(uint16_t address, uint8_t value); //return the number of clock cycles it takes
uint8_t readRAM(uint16_t address);

void writeVRAM(uint16_t address, uint8_t value);
uint8_t readVRAM(uint16_t address);

void writeSPRRAM(uint8_t address, uint8_t value);
uint8_t readSPRRAM(uint8_t address);

//debugging
void dumpRAM();
void dumpVRAM();
void dumpSPRRAM();

#endif // MMU_H_INCLUDED
