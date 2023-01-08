#ifndef CPU6502_H_INCLUDED
#define CPU6502_H_INCLUDED

#include <array>
#include <memory>

#include <stdint.h>

#include "filereader.h"
#include "ppu2C02.h"

class PPU2C02state;

#define N 7
#define V 6
#define UNDEFINED 5
#define B 4
#define D 3
#define I 2
#define Z 1
#define C 0

//all the different addressing modes on the 6502
enum addressingMode {
    NONE, //Undefined
    IMP, //Implied
    ACC, //Accumulator
    IMM, //Immediate
    ZER, //Zero Page
    ZERX, //Zero Page X
    ZERY, //Zero Page Y
    REL, //Relative
    ABS, //Absolute
    ABSX, //Absolute X
    ABSY, //Absoluete Y
    IND, //Indirect
    INXIND, //Indexed Indirect
    INDINX //Indirect Indexed
};

static const enum addressingMode addressModeLookup[256] =
{IMP, INXIND, NONE, INXIND, ZER, ZER, ZER, ZER, IMP, IMM, ACC, NONE, ABS, ABS, ABS, ABS, //00 -> 0F
 REL, INDINX, NONE, INDINX, ZERX, ZERX, ZERX, ZERX, IMP, ABSY, NONE, ABSY, ABSX, ABSX, ABSX, ABSX, //10 -> 1F
 ABS, INXIND, NONE, INXIND, ZER, ZER, ZER, ZER, IMP, IMM, ACC, NONE, ABS, ABS, ABS, ABS, //20 -> 2F
 REL, INDINX, NONE, INDINX, ZERX, ZERX, ZERX, ZERX, IMP, ABSY, NONE, ABSY, ABSX, ABSX, ABSX, ABSX, //30 -> 3F
 IMP, INXIND, NONE, INXIND, ZER, ZER, ZER, ZER, IMP, IMM, ACC, NONE, ABS, ABS, ABS, ABS, //40 -> 4F
 REL, INDINX, NONE, INDINX, ZERX, ZERX, ZERX, ZERX, IMP, ABSY, NONE, ABSY, ABSX, ABSX, ABSX, ABSX, //50 -> 5F
 IMP, INXIND, NONE, INXIND, ZER, ZER, ZER, ZER, IMP, IMM, ACC, NONE, IND, ABS, ABS, ABS, //60 -> 6F
 REL, INDINX, NONE, INDINX, ZERX, ZERX, ZERX, ZERX, IMP, ABSY, NONE, ABSY, ABSX, ABSX, ABSX, ABSX, //70 -> 7F
 IMM, INXIND, NONE, INXIND, ZER, ZER, ZER, ZER, IMP, NONE, IMP, NONE, ABS, ABS, ABS, ABS, //80 -> 8F
 REL, INDINX, NONE, NONE, ZERX, ZERX, ZERY, ZERY, IMP, ABSY, IMP, NONE, NONE, ABSX, NONE, NONE, //90 -> 9F
 IMM, INXIND, IMM, INXIND, ZER, ZER, ZER, ZER, IMP, IMM, IMP, NONE, ABS, ABS, ABS, ABS, //A0 -> AF
 REL, INDINX, NONE, INDINX, ZERX, ZERX, ZERY, ZERY, IMP, ABSY, IMP, NONE, ABSX, ABSX, ABSY, ABSY, //B0 -> BF
 IMM, INXIND, NONE, INXIND, ZER, ZER, ZER, ZER, IMP, IMM, IMP, NONE, ABS, ABS, ABS, ABS, //C0 -> CF
 REL, INDINX, NONE, INDINX, ZERX, ZERX, ZERX, ZERX, IMP, ABSY, NONE, ABSY, ABSX, ABSX, ABSX, ABSX, //D0 -> DF
 IMM, INXIND, NONE, INXIND, ZER, ZER, ZER, ZER, IMP, IMM, NONE, IMM, ABS, ABS, ABS, ABS, //E0 -> EF
 REL, INDINX, NONE, INDINX, ZERX, ZERX, ZERX, ZERX, IMP, ABSY, NONE, ABSY, ABSX, ABSX, ABSX, ABSX //F0 -> FF
 };

 static const int clockCycleLookup[256] =
 {
    7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, //00 -> 0F
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //10 -> 1F
    6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, //20 -> 2F
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //30 -> 3F
    6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, //40 -> 4F
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //50 -> 5F
    6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, //60 -> 6F
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //70 -> 7F
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 0, 4, 4, 4, 4, //80 -> 8F
    2, 6, 0, 0, 4, 4, 4, 4, 2, 5, 2, 0, 0, 5, 0, 0, //90 -> 9F
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 0, 4, 4, 4, 4, //A0 -> AF
    2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 0, 4, 4, 4, 4, //B0 -> BF
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, //C0 -> CF
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, //D0 -> DF
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, //E0 -> EF
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7 //F0 -> FF
 };

static const int pageBreakingLookup[256] =
 {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //00 -> 0F
    0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //10 -> 1F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20 -> 2F
    0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //30 -> 3F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, //40 -> 4F
    0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //50 -> 5F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //60 -> 6F
    0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //70 -> 7F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //80 -> 8F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //90 -> 9F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //A0 -> AF
    0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, //B0 -> BF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //C0 -> CF
    0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, //D0 -> DF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //E0 -> EF
    0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0 //F0 -> FF
 };

// class to store the state of the processor
class CPU6502state {
    public:
        CPU6502state(PPU2C02state *ppu, std::shared_ptr<Cartridge> cartridge);

        std::array<uint8_t, 0x800> ram;
        uint16_t PC; //program counter (16 bits)
        uint16_t SP; //stack pointer (16 bits)
        uint8_t A; //accumulator register (8 bits)
        uint8_t X; // (8 bits)
        uint8_t Y; // (8 bits)
        uint8_t P; //status register (8 bits) NV_BDIZC

        uint8_t ReadRam(uint16_t address);
        uint8_t writeRAM(uint16_t address, uint8_t value);

        std::shared_ptr<Cartridge> cartridge;
        PPU2C02state* ppu;

        //fetches and executes an opcode
        int updateZN(uint8_t variable);
        uint8_t fetchAndExecute();

        //interrupts
        void NMI();

        //CPU addressing modes (implemented in cpu6502instructions.c)
        uint16_t getAddress(uint8_t opcode);

        uint16_t addressImmediate();
        uint16_t addressZeroPage();
        uint16_t addressZeroPageX();
        uint16_t addressZeroPageY();
        int8_t addressRelative();
        uint16_t addressAbsolute();
        uint16_t addressAbsoluteX();
        uint16_t addressAbsoluteY();
        uint16_t addressIndirect();
        uint16_t addressIndexedIndirect();
        uint16_t addressIndirectIndexed();

        int updateCCompare(int var1, int var2);
        uint8_t isPageBreaking(uint8_t opcode);

        void pushStack(int value);
        uint8_t popStack();

        //CPU instructions (implemented in cpu6502instructions.c)
        void ADC(uint8_t argument);
        void SBC(uint8_t argument);
        uint8_t LSR(uint8_t argument);
        uint8_t ASL(uint8_t argument);
        uint8_t ROR(uint8_t argument);
        uint8_t ROL(uint8_t argument);
        void DEC(uint16_t argument);
        void INC(uint16_t argument);
        void SLO(uint16_t argument);
        void RLA(uint16_t argument);
        void RRA(uint16_t argument);
        void SRE(uint16_t argument);
        uint8_t BXX(uint16_t address, int flag, bool shouldBeSet);
};

#endif // CPU6502_H_INCLUDED
