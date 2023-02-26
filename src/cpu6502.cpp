#include <stdlib.h>
#include <stdio.h>

#include "controller.h"
#include "cpu6502.h"
#include "ppu2C02.h"

CPU6502state::CPU6502state(PPU2C02state *ppu, std::shared_ptr<Mapper> mapper) {
    this->mapper = mapper;

    PC = 0xC000;
    SP = 0xFD;
    X = 0;
    Y = 0;
    P = 0x24;
    A = 0;

    this->ppu = ppu;
    this->ppu->SetMapper(mapper);

    uint8_t high = ReadRam(0xFFFD);
    uint8_t low = ReadRam(0xFFFC);
    PC = (high << 8) | low;
    this->clock_cycle = 7;
    //this->PC = 0xc000;

    printf("pc: %X %x %x\n", PC, high, low);
}

int CPU6502state::updateZN(uint8_t variable) {
    P &= ~(1<<Z);

    if(variable == 0) {
        P |= (1<<Z);
    }

    P &= ~(1<<N);
    if(variable & (1 << 7)) {
        P |= (1<<N);
    }

    return 0;
}

int CPU6502state::updateCCompare(int var1, int var2) {
    P &= ~(1<<C);
    if(var1>=var2) {
        P |= (1<<C);
    }

    return 0;
}

void CPU6502state::Tick() {
    clock_cycle += 1;

    apu.clock();
    ppu->PPUcycle();
    ppu->PPUcycle();
    ppu->PPUcycle();
}

/******************
* stack operations
******************/
void CPU6502state::pushStack(int value) {
    WriteRam( 0x100 + SP, value );
    SP -= 1;
    SP &= 0xFF;
}

uint8_t CPU6502state::popStack() {
    SP += 1;
    SP &= 0xFF;
    return ReadRam( (0x100 + SP)&0x1FF );
}

/******************
* interrupts
******************/
void CPU6502state::NMI() {
    uint8_t low = ReadRam(0xFFFA);
    uint8_t high = ReadRam(0xFFFB);
    uint16_t address = (high << 8) | low;

    pushStack(PC >> 8);
    pushStack(PC & 0xFF);
    pushStack(P);

    PC = address;
}

/******************
* Fetches and executes an operating code
******************/
uint8_t CPU6502state::fetchAndExecute() {

    //debugging variables
    int oldPC = PC;
    int oldA = A;
    int oldX = X;
    int oldY = Y;
    int oldP = P;
    int oldSP = SP;
    char c1[3] = "  ";
    char c2[3] = "  ";
    char c3[3] = "  ";
    char instruction[4] = "   ";
    char after[27] = "                          ";

    if(ppu->nmi) {
        NMI();
        ppu->nmi = false;
    }

    std::string op_str = "";
    uint clock_cycles_before = this->clock_cycle;

    uint8_t opcode = ReadRam( PC++ );
    switch(opcode) {
        /***********************
        ** REGISTER OPERATIONS
        ***********************/
        case 0x4C: op_str="JMP"; JMP(addressAbsolute()); break;
        case 0x6C: op_str="JMP"; JMP(addressIndirect()); break;

        case 0xA0: op_str="LDY"; LD(Y, addressImmediate()); break;
        case 0xA4: op_str="LDY"; LD(Y, addressZeroPage()); break;
        case 0xAC: op_str="LDY"; LD(Y, addressAbsolute()); break;
        case 0xB4: op_str="LDY"; LD(Y, addressZeroPageX()); break;
        case 0xBC: op_str="LDY"; LD(Y, addressAbsoluteX()); break;

        case 0xA2: op_str="LDX"; LD(X, addressImmediate()); break;
        case 0xA6: op_str="LDX"; LD(X, addressZeroPage()); break;
        case 0xB6: op_str="LDX"; LD(X, addressZeroPageY()); break;
        case 0xBE: op_str="LDX"; LD(X, addressAbsoluteY()); break;
        case 0xAE: op_str="LDX"; LD(X, addressAbsolute()); break;

        case 0xA1: op_str="LDA"; LD(A, addressIndexedIndirect()); break;
        case 0xA5: op_str="LDA"; LD(A, addressZeroPage()); break;
        case 0xA9: op_str="LDA"; LD(A, addressImmediate()); break;
        case 0xAD: op_str="LDA"; LD(A, addressAbsolute()); break;
        case 0xB1: op_str="LDA"; LD(A, addressIndirectIndexed()); break;
        case 0xB5: op_str="LDA"; LD(A, addressZeroPageX()); break;
        case 0xB9: op_str="LDA"; LD(A, addressAbsoluteY()); break;
        case 0xBD: op_str="LDA"; LD(A, addressAbsoluteX()); break;

        case 0x84: op_str="STY"; ST(Y, addressZeroPage()); break;
        case 0x8C: op_str="STY"; ST(Y, addressAbsolute()); break;
        case 0x94: op_str="STY"; ST(Y, addressZeroPageX()); break;

        case 0x86: op_str="STX"; ST(X, addressZeroPage()); break;
        case 0x8E: op_str="STX"; ST(X, addressAbsolute()); break;
        case 0x96: op_str="STX"; ST(X, addressZeroPageY()); break;

        case 0x85: op_str="STA"; ST(A, addressZeroPage()); break;
        case 0x95: op_str="STA"; ST(A, addressZeroPageX()); break;
        case 0x8D: op_str="STA"; ST(A, addressAbsolute()); break;
        case 0x9D: op_str="STA"; Tick(); ST(A, addressAbsolute()+X); break;
        case 0x99: op_str="STA"; Tick(); ST(A, addressAbsolute()+Y); break;
        case 0x81: op_str="STA"; ST(A, addressIndexedIndirect()); break;
        case 0x91: op_str="STA"; Tick(); ST(A, addressIndirectIndexed()); break;

        case 0x20: op_str="JSR"; JSR(addressAbsolute()); break;

        case 0x38: op_str="SEC"; SE(Flags::C); break;
        case 0xF8: op_str="SED"; SE(Flags::D); break;
        case 0x78: op_str="SEI"; SE(Flags::I); break;

        case 0x58: op_str="CLI"; CL(Flags::I); break;
        case 0xD8: op_str="CLD"; CL(Flags::D); break;
        case 0x18: op_str="CLC"; CL(Flags::C); break;
        case 0xB8: op_str="CLV"; CL(Flags::V); break;

        case 0xD0: op_str="BNE"; Branch(addressRelative(), Z, false); break;
        case 0xF0: op_str="BEQ"; Branch(addressRelative(), Z, true); break;
        case 0xB0: op_str="BCS"; Branch(addressRelative(), C, true); break;
        case 0x90: op_str="BCC"; Branch(addressRelative(), C, false); break;
        case 0x70: op_str="BVS"; Branch(addressRelative(), V, true); break;
        case 0x50: op_str="BVC"; Branch(addressRelative(), V, false); break;
        case 0x30: op_str="BMI"; Branch(addressRelative(), N, true); break;
        case 0x10: op_str="BPL"; Branch(addressRelative(), N, false); break;

        case 0x24: op_str="BIT"; BIT(addressZeroPage()); break;
        case 0x2C: op_str="BIT"; BIT(addressAbsolute()); break;

        case 0x60: op_str="RTS"; RTS(); break;

        case 0x8A: op_str="TXA"; Tick(); A = X; updateZN(A); break;
        case 0x98: op_str="TYA"; Tick(); A = Y; updateZN(A); break;
        case 0x9A: op_str="TXS"; Tick(); SP = X; break;
        case 0xA8: op_str="TAY"; Tick(); Y = A; updateZN(Y); break;
        case 0xAA: op_str="TAX"; Tick(); X = A; updateZN(X); break;
        case 0xBA: op_str="TSX"; Tick(); X = SP; updateZN(X); break;

        case 0xCA: op_str="DEX"; Tick(); X -= 1; updateZN(X); break;
        case 0x88: op_str="DEY"; Tick(); Y -= 1; updateZN(Y); break;

        case 0xE8: op_str="INX"; Tick(); X += 1; updateZN(X); break;
        case 0xC8: op_str="INY"; Tick(); Y += 1; updateZN(Y); break;

        case 0x29: op_str="AND"; AND(addressImmediate()); break;
        case 0x25: op_str="AND"; AND(addressZeroPage()); break;
        case 0x35: op_str="AND"; AND(addressZeroPageX()); break;
        case 0x2D: op_str="AND"; AND(addressAbsolute()); break;
        case 0x3D: op_str="AND"; AND(addressAbsoluteX()); break;
        case 0x39: op_str="AND"; AND(addressAbsoluteY()); break;
        case 0x21: op_str="AND"; AND(addressIndexedIndirect()); break;
        case 0x31: op_str="AND"; AND(addressIndirectIndexed()); break;

        case 0xC9: op_str="CMP"; Compare(A, addressImmediate()); break;
        case 0xC5: op_str="CMP"; Compare(A, addressZeroPage()); break;
        case 0xD5: op_str="CMP"; Compare(A, addressZeroPageX()); break;
        case 0xCD: op_str="CMP"; Compare(A, addressAbsolute()); break;
        case 0xDD: op_str="CMP"; Compare(A, addressAbsoluteX()); break;
        case 0xD9: op_str="CMP"; Compare(A, addressAbsoluteY()); break;
        case 0xC1: op_str="CMP"; Compare(A, addressIndexedIndirect()); break;
        case 0xD1: op_str="CMP"; Compare(A, addressIndirectIndexed()); break;

        case 0x09: op_str="ORA"; ORA(addressImmediate()); break;
        case 0x05: op_str="ORA"; ORA(addressZeroPage()); break;
        case 0x15: op_str="ORA"; ORA(addressZeroPageX()); break;
        case 0x0D: op_str="ORA"; ORA(addressAbsolute()); break;
        case 0x1D: op_str="ORA"; ORA(addressAbsoluteX()); break;
        case 0x19: op_str="ORA"; ORA(addressAbsoluteY()); break;
        case 0x01: op_str="ORA"; ORA(addressIndexedIndirect()); break;
        case 0x11: op_str="ORA"; ORA(addressIndirectIndexed()); break;

        case 0x49: op_str="EOR"; EOR(addressImmediate()); break;
        case 0x45: op_str="EOR"; EOR(addressZeroPage()); break;
        case 0x55: op_str="EOR"; EOR(addressZeroPageX()); break;
        case 0x4D: op_str="EOR"; EOR(addressAbsolute()); break;
        case 0x5D: op_str="EOR"; EOR(addressAbsoluteX()); break;
        case 0x59: op_str="EOR"; EOR(addressAbsoluteY()); break;
        case 0x41: op_str="EOR"; EOR(addressIndexedIndirect()); break;
        case 0x51: op_str="EOR"; EOR(addressIndirectIndexed()); break;

        case 0x69: op_str="ADC"; ADC(addressImmediate()); break;
        case 0x65: op_str="ADC"; ADC(addressZeroPage()); break;
        case 0x75: op_str="ADC"; ADC(addressZeroPageX()); break;
        case 0x6D: op_str="ADC"; ADC(addressAbsolute()); break;
        case 0x7D: op_str="ADC"; ADC(addressAbsoluteX()); break;
        case 0x79: op_str="ADC"; ADC(addressAbsoluteY()); break;
        case 0x61: op_str="ADC"; ADC(addressIndexedIndirect()); break;
        case 0x71: op_str="ADC"; ADC(addressIndirectIndexed()); break;

        case 0xE9: op_str="SBC"; SBC(addressImmediate()); break;
        case 0xE5: op_str="SBC"; SBC(addressZeroPage()); break;
        case 0xF5: op_str="SBC"; SBC(addressZeroPageX()); break;
        case 0xED: op_str="SBC"; SBC(addressAbsolute()); break;
        case 0xFD: op_str="SBC"; SBC(addressAbsoluteX()); break;
        case 0xF9: op_str="SBC"; SBC(addressAbsoluteY()); break;
        case 0xE1: op_str="SBC"; SBC(addressIndexedIndirect()); break;
        case 0xF1: op_str="SBC"; SBC(addressIndirectIndexed()); break;
        case 0xEB: op_str="SBC"; SBC(addressImmediate()); break;

        case 0xC0: op_str="CPY"; Compare(Y, addressImmediate()); break;
        case 0xC4: op_str="CPY"; Compare(Y, addressZeroPage()); break;
        case 0xCC: op_str="CPY"; Compare(Y, addressAbsolute()); break;

        case 0xE0: op_str="CPX"; Compare(X, addressImmediate()); break;
        case 0xE4: op_str="CPX"; Compare(X, addressZeroPage()); break;
        case 0xEC: op_str="CPX"; Compare(X, addressAbsolute()); break;

        case 0x08: op_str="PHP"; Tick(); pushStack(P | (1<<UNDEFINED) | (1<<B)); break;
        case 0x28: op_str="PLP"; Tick(); Tick(); P = ((popStack() & ~(1<<B)) | (1<<UNDEFINED)); break;
        case 0x48: op_str="PHA"; Tick(); pushStack(A); break;
        case 0x68: op_str="PLA"; Tick(); Tick(); A = popStack(); updateZN(A); break;

        case 0x0A: op_str="ASL"; A = LeftShift(A); Tick(); break;
        case 0x06: op_str="ASL"; ASL(addressZeroPage()); break;
        case 0x16: op_str="ASL"; ASL(addressZeroPageX()); break;
        case 0x0E: op_str="ASL"; ASL(addressAbsolute()); break;
        case 0x1E: op_str="ASL"; ASL(addressAbsolute() + X); Tick(); break;

        case 0x4A: op_str="LSR"; A = RightShift(A); Tick(); break;
        case 0x46: op_str="LSR"; LSR(addressZeroPage()); break;
        case 0x56: op_str="LSR"; LSR(addressZeroPageX()); break;
        case 0x4E: op_str="LSR"; LSR(addressAbsolute()); break;
        case 0x5E: op_str="LSR"; LSR(addressAbsolute() + X); Tick(); break;

        case 0x6A: op_str="ROR"; A = RightRotate(A); Tick(); break;
        case 0x66: op_str="ROR"; ROR(addressZeroPage()); break;
        case 0x76: op_str="ROR"; ROR(addressZeroPageX()); break;
        case 0x6E: op_str="ROR"; ROR(addressAbsolute()); break;
        case 0x7E: op_str="ROR"; Tick(); ROR(addressAbsolute() + X); break;

        case 0x2A: op_str="ROL"; A = LeftRotate(A); Tick(); break;
        case 0x26: op_str="ROL"; ROL(addressZeroPage()); break;
        case 0x36: op_str="ROL"; ROL(addressZeroPageX()); break;
        case 0x2E: op_str="ROL"; ROL(addressAbsolute()); break;
        case 0x3E: op_str="ROL"; ROL(addressAbsolute() + X); Tick(); break;

        case 0xE6: op_str="INC"; INC(addressZeroPage()); break;
        case 0xF6: op_str="INC"; INC(addressZeroPageX()); break;
        case 0xEE: op_str="INC"; INC(addressAbsolute()); break;
        case 0xFE: op_str="INC"; INC(addressAbsolute() + X); Tick(); break;

        case 0xC6: op_str="DEC"; DEC(addressZeroPage()); break;
        case 0xD6: op_str="DEC"; DEC(addressZeroPageX()); break;
        case 0xCE: op_str="DEC"; DEC(addressAbsolute()); break;
        case 0xDE: op_str="DEC"; DEC(addressAbsolute() + X); Tick(); break;

        case 0x40: op_str="RTI"; RTI(); break;

        case 0x00: op_str="BRK"; BRK(); break;

        /***********************
        ** UNOFFICIAL OPCODES
        ***********************/

        case 0xA7: op_str="LAX"; LAX(addressZeroPage()); break;
        case 0xB7: op_str="LAX"; LAX(addressZeroPageY()); break;
        case 0xAF: op_str="LAX"; LAX(addressAbsolute()); break;
        case 0xBF: op_str="LAX"; LAX(addressAbsoluteY()); break;
        case 0xA3: op_str="LAX"; LAX(addressIndexedIndirect()); break;
        case 0xB3: op_str="LAX"; LAX(addressIndirectIndexed()); break;

        case 0x87: op_str="LAX"; WriteRam(addressZeroPage(), A&X); break;
        case 0x97: op_str="LAX"; WriteRam(addressZeroPageY(), A&X); break;
        case 0x8F: op_str="LAX"; WriteRam(addressAbsolute(), A&X); break;
        case 0x83: op_str="LAX"; WriteRam(addressIndexedIndirect(), A&X); break;

        case 0xC7: op_str="DCP"; DCP(addressZeroPage()); break;
        case 0xD7: op_str="DCP"; DCP(addressZeroPageX()); break;
        case 0xCF: op_str="DCP"; DCP(addressAbsolute()); break;
        case 0xDF: op_str="DCP"; DCP(addressAbsoluteX()); break;
        case 0xDB: op_str="DCP"; DCP(addressAbsoluteY()); break;
        case 0xC3: op_str="DCP"; DCP(addressIndexedIndirect()); break;
        case 0xD3: op_str="DCP"; DCP(addressIndirectIndexed()); break;

        case 0xE7: op_str="ISB"; ISB(addressZeroPage()); break;
        case 0xF7: op_str="ISB"; ISB(addressZeroPageX()); break;
        case 0xEF: op_str="ISB"; ISB(addressAbsolute()); break;
        case 0xFF: op_str="ISB"; ISB(addressAbsoluteX()); break;
        case 0xFB: op_str="ISB"; ISB(addressAbsoluteY()); break;
        case 0xE3: op_str="ISB"; ISB(addressIndexedIndirect()); break;
        case 0xF3: op_str="ISB"; ISB(addressIndirectIndexed()); break;

        case 0x07: op_str="SLO"; SLO(addressZeroPage()); break;
        case 0x17: op_str="SLO"; SLO(addressZeroPageX()); break;
        case 0x0F: op_str="SLO"; SLO(addressAbsolute()); break;
        case 0x1F: op_str="SLO"; SLO(addressAbsoluteX()); break;
        case 0x1B: op_str="SLO"; SLO(addressAbsoluteY()); break;
        case 0x03: op_str="SLO"; SLO(addressIndexedIndirect()); break;
        case 0x13: op_str="SLO"; SLO(addressIndirectIndexed()); break;

        case 0x27: op_str="RLA"; RLA(addressZeroPage()); break;
        case 0x37: op_str="RLA"; RLA(addressZeroPageX()); break;
        case 0x2F: op_str="RLA"; RLA(addressAbsolute()); break;
        case 0x3F: op_str="RLA"; RLA(addressAbsoluteX()); break;
        case 0x3B: op_str="RLA"; RLA(addressAbsoluteY()); break;
        case 0x23: op_str="RLA"; RLA(addressIndexedIndirect()); break;
        case 0x33: op_str="RLA"; RLA(addressIndirectIndexed()); break;

        case 0x47: op_str="SRE"; SRE(addressZeroPage()); break;
        case 0x57: op_str="SRE"; SRE(addressZeroPageX()); break;
        case 0x4F: op_str="SRE"; SRE(addressAbsolute()); break;
        case 0x5F: op_str="SRE"; SRE(addressAbsoluteX()); break;
        case 0x5B: op_str="SRE"; SRE(addressAbsoluteY()); break;
        case 0x43: op_str="SRE"; SRE(addressIndexedIndirect()); break;
        case 0x53: op_str="SRE"; SRE(addressIndirectIndexed()); break;

        case 0x67: op_str="RRA"; RRA(addressZeroPage()); break;
        case 0x77: op_str="RRA"; RRA(addressZeroPageX()); break;
        case 0x6F: op_str="RRA"; RRA(addressAbsolute()); break;
        case 0x7F: op_str="RRA"; RRA(addressAbsoluteX()); break;
        case 0x7B: op_str="RRA"; RRA(addressAbsoluteY()); break;
        case 0x63: op_str="RRA"; RRA(addressIndexedIndirect()); break;
        case 0x73: op_str="RRA"; RRA(addressIndirectIndexed()); break;

        case 0x04: op_str="NOP"; addressZeroPage(); Tick(); break;
        case 0x44: op_str="NOP"; addressZeroPage(); Tick(); break;
        case 0x64: op_str="NOP"; addressZeroPage(); Tick(); break;
        case 0x14: op_str="NOP"; addressZeroPageX(); Tick(); break;
        case 0x34: op_str="NOP"; addressZeroPageX(); Tick(); break;
        case 0x54: op_str="NOP"; addressZeroPageX(); Tick(); break;
        case 0x74: op_str="NOP"; addressZeroPageX(); Tick(); break;
        case 0xD4: op_str="NOP"; addressZeroPageX(); Tick(); break;
        case 0xF4: op_str="NOP"; addressZeroPageX(); Tick(); break;
        case 0x0C: op_str="NOP"; addressAbsolute(); Tick(); break;
        case 0x1C: op_str="NOP"; addressAbsoluteX(); Tick(); break;
        case 0x3C: op_str="NOP"; addressAbsoluteX(); Tick(); break;
        case 0x5C: op_str="NOP"; addressAbsoluteX(); Tick(); break;
        case 0x7C: op_str="NOP"; addressAbsoluteX(); Tick(); break;
        case 0xDC: op_str="NOP"; addressAbsoluteX(); Tick(); break;
        case 0xFC: op_str="NOP"; addressAbsoluteX(); Tick(); break;
        case 0x80: op_str="NOP"; addressImmediate(); Tick(); break;

        default: op_str="NOP"; Tick(); break;
    }

    //print debug info
    //sprintf(c1, "%02X", opcode);
    //printf("%04X  %s %s %s  %s %s  A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%d\n", oldPC, c1, c2, c3, op_str.c_str(), after, oldA, oldX, oldY, oldP, oldSP, clock_cycles_before);
    uint clock_cycles_after = this->clock_cycle;
    return clock_cycles_after-clock_cycles_before;
}

uint8_t CPU6502state::WriteRam(uint16_t address, uint8_t value) {
    Tick();

    if(address <= 0x1FFF) {
        ram[address%0x0800] = value;
    }
    else if(address <= 0x3FFF) {
        return ppu->writeRegisters(0x2000 + (address%8), value);
    }
    else if(address <= 0x4013 || address == 0x4015 || address == 0x4017) {
        apu.writeRegister(address, value);
    }
    else if(address == 0x4014) {
        for(int i=0; i<=0xFF; ++i) {
            ppu->writeSPRRAM(i, ram.at((value << 8)|i) );
        }
        return 513; //TODO: odd cpu cycles takes one extra cycle
    }
    else if(address <= 0x4017) {
        if(address == 0x4016) {
            writeController(&NES_Controller, value);
        }
        else if(address == 0x4017) {
            // Controller 2 not emulated
        }
    }
    else if (address >= 0x4020) {
        mapper->WritePrg(address, value);
    }

    return 0;
}

uint8_t CPU6502state::ReadRam(uint16_t address) {
    Tick();

    if(address <= 0x1FFF) {
        return ram[address%0x0800];
    }
    else if(address <= 0x3FFF) {
        return ppu->readRegisters(0x2000 + (address%8));
    }
    else if(address <= 0x4014) {
        // Open bus behaviour not emulated
    }
    else if(address == 0x4015) {
       // APU not emulated
    }
    else if(address <= 0x4017) {
        if(address == 0x4016) {
            return getNextButton(&NES_Controller);
        }
        else if(address == 0x4017) {
            // Controller 2 not emulated
        }
    }
    else if (address >= 0x4020) {
        return mapper->ReadPrg(address);
    }
    return 0;
}