#ifndef CPU6502_H_INCLUDED
#define CPU6502_H_INCLUDED

#include <array>
#include <memory>

#include <stdint.h>

#include "filereader.h"
#include "ppu2C02.h"
#include "apu.h"

class PPU2C02state;

enum Flags {
    N=7, V=6, UNDEFINED=5, B=4, D=3, I=2, Z=1, C=0
};

// class to store the state of the processor
class CPU6502state {
    public:
        CPU6502state(PPU2C02state *ppu, std::shared_ptr<Mapper> mapper);

        std::array<uint8_t, 0x800> ram;
        uint16_t PC; //program counter (16 bits)
        uint16_t SP; //stack pointer (16 bits)
        uint8_t A; //accumulator register (8 bits)
        uint8_t X; // (8 bits)
        uint8_t Y; // (8 bits)
        uint8_t P; //status register (8 bits) NV_BDIZC

        uint8_t ReadRam(uint16_t address);
        uint8_t WriteRam(uint16_t address, uint8_t value);

        std::shared_ptr<Mapper> mapper;
        PPU2C02state* ppu;

        //fetches and executes an opcode
        int updateZN(uint8_t variable);
        uint8_t fetchAndExecute();

        //interrupts
        void NMI();

        //CPU addressing modes (implemented in cpu6502instructions.c)
        uint16_t addressImmediate();
        uint16_t addressZeroPage();
        uint16_t addressZeroPageX();
        uint16_t addressZeroPageY();
        uint16_t addressRelative();
        uint16_t addressAbsolute();
        uint16_t addressAbsoluteX();
        uint16_t addressAbsoluteY();
        uint16_t addressIndirect();
        uint16_t addressIndexedIndirect();
        uint16_t addressIndirectIndexed();

        int updateCCompare(int var1, int var2);

        void pushStack(int value);
        uint8_t popStack();

        Apu apu;

        uint8_t done_render = 0;

    private:
        uint clock_cycle = 0;

        void Tick();

        // Instructions (implemented in cpu6502instructions.c)
        void ADC(const uint16_t &address);
        void AND(const uint16_t &address);
        void ASL(const uint16_t &address);
        void Branch(const uint16_t &address, const Flags &flag, bool shouldBeSet);
        void BRK();
        void BIT(const uint16_t &address);
        void CL(const Flags &flag); // CLC, CLD, CLI, CLV
        void Compare(uint8_t &reg, const uint16_t &address);
        void DCP(const uint16_t &address);
        void DEC(const uint16_t &address);
        void EOR(const uint16_t &address);
        void INC(const uint16_t &address);
        void ISB(const uint16_t &address);
        void JMP(const uint16_t &address);
        void JSR(const uint16_t &address);
        void LAX(const uint16_t &address);
        void LSR(const uint16_t &address);
        void LD(uint8_t &reg, const uint16_t &address); // LDA, LDX, LDY
        void ORA(const uint16_t &address);
        void RLA(const uint16_t &address);
        void ROL(const uint16_t &address);
        void ROR(const uint16_t &address);
        void RRA(const uint16_t &address);
        void RTI(); 
        void RTS();
        void SBC(const uint16_t &address);
        void SE(const Flags &flag); // SEC, SED, SEI
        void SLO(const uint16_t &address);
        void SRE(const uint16_t &address);
        void ST(uint8_t &reg, const uint16_t &address); // STA, STX, STY

        // Utils (implemented in cpu6502instructions.c)
        uint8_t RightShift(const uint8_t &argument);
        uint8_t LeftShift(const uint8_t &argument);
        uint8_t RightRotate(const uint8_t &argument);
        uint8_t LeftRotate(const uint8_t &argument);

        bool CrossedPage(const uint16_t &address, const uint8_t &increment);
};

#endif // CPU6502_H_INCLUDED
