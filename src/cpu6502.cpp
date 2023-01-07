#include <stdlib.h>
#include <stdio.h>

#include "controller.h"
#include "cpu6502.h"
#include "ppu2C02.h"

CPU6502state CPU_state;

int initCPU6502(CPU6502state *state, Cartridge cartridge, std::shared_ptr<PPU2C02state> ppu) {
    state->cartridge = cartridge;

    state->PC = 0xC000;
    state->SP = 0xFD;
    state->X = 0;
    state->Y = 0;
    state->P = 0x24;
    state->A = 0;

    uint8_t high = state->readRAM(0xFFFD);
    uint8_t low = state->readRAM(0xFFFC);
    state->PC = (high << 8) | low;
    state->ppu = ppu;

    //printf("pc: %X %x %x\n", state->PC, high, low);

    return 1;
}

int updateZN(CPU6502state *state, uint8_t variable) {
    state->P &= ~(1<<Z);

    if(variable == 0) {
        state->P |= (1<<Z);
    }

    state->P &= ~(1<<N);
    if(variable & (1 << 7)) {
        state->P |= (1<<N);
    }

    return 0;
}

int updateCCompare(CPU6502state *state, int var1, int var2) {
    state->P &= ~(1<<C);
    if(var1>=var2) {
        state->P |= (1<<C);
    }

    return 0;
}

//checks if running an opcode in a given state would cross a page boundary
uint8_t isPageBreaking(CPU6502state *state, uint8_t opcode) {
    uint32_t address, low, high;
    enum addressingMode mode = addressModeLookup[opcode];
    switch(mode) {
        case ABSX:
            low = state->readRAM(state->PC);
            high = state->readRAM(state->PC+1);
            if ( ((((high << 8) | low) + state->X) & 0xFF00) != (((high << 8) | low) & 0xFF00) ) {
                return 1;
            }
            return 0;
        case ABSY:
            low = state->readRAM(state->PC);
            high = state->readRAM(state->PC+1);
            if ( ((((high << 8) | low) + state->Y) & 0xFF00) != (((high << 8) | low) & 0xFF00) ) {
                return 1;
            }
            return 0;
        case INDINX:
            address = state->readRAM(state->PC);
            low = state->readRAM(address++);
            high = state->readRAM( address & 0xFF );

            if( ((((high << 8) | low) + state->Y) &0xFF00) != (((high << 8) | low) & 0xFF00) ) {
                return 1;
            }

            return 0;
        default:
            return 0;
    }
}

/******************
* stack operations
******************/
void pushStack(CPU6502state* state, int value) {
    state->writeRAM( 0x100 + state->SP, value );
    state->SP -= 1;
    state->SP &= 0xFF;
}

uint8_t popStack(CPU6502state* state) {
    state->SP += 1;
    state->SP &= 0xFF;
    return state->readRAM( (0x100 + state->SP)&0x1FF );
}

/******************
* interrupts
******************/
void NMI(CPU6502state *state) {
    uint8_t low = state->readRAM(0xFFFA);
    uint8_t high = state->readRAM(0xFFFB);
    uint16_t address = (high << 8) | low;

    pushStack(state, state->PC >> 8);
    pushStack(state, state->PC & 0xFF);
    pushStack(state, state->P);

    state->PC = address;
}

/******************
* Fetches and executes an operating code
******************/
uint8_t fetchAndExecute(CPU6502state *state) {

    //debugging variables
    int oldPC = state->PC;
    int oldA = state->A;
    int oldX = state->X;
    int oldY = state->Y;
    int oldP = state->P;
    int oldSP = state->SP;
    char c1[3] = "  ";
    char c2[3] = "  ";
    char c3[3] = "  ";
    char instruction[4] = "   ";
    char after[27] = "                          ";

    uint8_t opcode = state->readRAM( state->PC++ );
    uint8_t clockCycles = clockCycleLookup[opcode];
    if(isPageBreaking(state, opcode)) {
        uint8_t add = pageBreakingLookup[opcode];
        clockCycles += add;
    }
    uint16_t address = getAddress(state, opcode) ;

    switch(opcode) {
        /***********************
        ** REGISTER OPERATIONS
        ***********************/
        //LDY
        case 0xA0: case 0xA4: case 0xB4: case 0xAC:
        case 0xBC:{
            state->Y = state->readRAM(address);
            updateZN(state, state->Y);
            sprintf(instruction, "LDY");
            break;}

        //STY
        case 0x84: case 0x8C: case 0x94:{
            clockCycles += state->writeRAM(address, state->Y);
            sprintf(instruction, "STY");
            break;}

        //LDX
        case 0xA2: case 0xA6: case 0xB6: case 0xAE:
        case 0xBE:{
            state->X = state->readRAM(address);
            updateZN(state, state->X);
            sprintf(instruction, "LDX");
            break;}

        //STX
        case 0x86: case 0x8E: case 0x96:{
            clockCycles += state->writeRAM(address, state->X);
            sprintf(instruction, "STX");
            break;}

        //TXA Implied
        case 0x8A:{
            state->A = state->X;
            updateZN(state, state->A);
            sprintf(instruction, "TXA");
            break;}

        //TYA Implied
        case 0x98:{
            state->A = state->Y;
            updateZN(state, state->A);
            sprintf(instruction, "TYA");
            break;}

        //TXS Implied
        case 0x9A:{
            state->SP = state->X;
            sprintf(instruction, "TXS");
            break;}

        //TAY Implied
        case 0xA8:{
            state->Y = state->A;
            updateZN(state, state->Y);
            sprintf(instruction, "TAY");
            break;}

        //LDA Immediate
        case 0xA9: case 0xA5: case 0xB5: case 0xAD: case 0xBD: case 0xB9: case 0xA1:
        case 0xB1:{
            state->A = state->readRAM(address);
            updateZN(state, state->A);
            sprintf(instruction, "LDA");
            break;}

        //TAX Implied
        case 0xAA:{
            state->X = state->A;
            updateZN(state, state->X);
            sprintf(instruction, "TAX");
            break;}

        //TSX Implied
        case 0xBA:{
            state->X = state->SP;
            updateZN(state, state->X);
            sprintf(instruction, "TSX");
            break;}

        //BIT
        case 0x24:
        case 0x2C:{
            int inMemory = state->readRAM(address);
            updateZN(state, inMemory&state->A);
            state->P &= ~(1<<V);
            if(inMemory & (1 << 6)) state->P |= (1<<V);
            state->P &= ~(1<<N);
            if(inMemory & (1 << 7)) state->P |= (1<<N);
            sprintf(instruction, "BIT");
            break;}

        //INY Implied
        case 0xC8:{
            state->Y += 1;
            updateZN(state, state->Y);
            sprintf(instruction, "INY");
            break;}

        //DEY Implied
        case 0x88:{
            state->Y -= 1;
            updateZN(state, state->Y);
            sprintf(instruction, "DEY");
            break;}

        //INX Implied
        case 0xE8:{
            state->X += 1;
            updateZN(state, state->X);
            sprintf(instruction, "INX");
            break;}

        //DEX Implied
        case 0xCA:{
            state->X -= 1;
            updateZN(state, state->X);
            sprintf(instruction, "DEX");
            break;}

        /***********************
        ** ARITHMETIC OPERATIONS
        ***********************/
        //AND
        case 0x29: case 0x25: case 0x35: case 0x2D:
        case 0x3D: case 0x39: case 0x21: case 0x31:{
            state->A &= state->readRAM(address);
            updateZN(state, state->A);
            sprintf(instruction, "AND");
            break;}

        //CMP
        case 0xC9: case 0xC5: case 0xD5: case 0xCD:
        case 0xDD: case 0xD9: case 0xC1: case 0xD1:{
            int comparator = state->readRAM(address);
            updateCCompare(state, state->A,comparator);
            updateZN(state, state->A-comparator);
            sprintf(instruction, "CMP");
            break;}

        //ORA
        case 0x09: case 0x05: case 0x15: case 0x0D: case 0x1D:
        case 0x19: case 0x01: case 0x11:{
            state->A |= state->readRAM(address);
            updateZN(state, state->A);
            sprintf(instruction, "ORA");
            break;}

        //EOR
        case 0x49: case 0x45: case 0x55: case 0x4D: case 0x5D:
        case 0x59: case 0x41: case 0x51:{
            state->A ^= state->readRAM(address);
            updateZN(state, state->A);
            sprintf(instruction, "EOR");
            break;}

        //ADC
        case 0x69: case 0x65: case 0x75: case 0x6D: case 0x7D:
        case 0x79: case 0x61: case 0x71:{
            ADC(state, state->readRAM(address) );
            sprintf(instruction, "ADC");
            break;}

        //SBC Immediate
        case 0xEB: case 0xE9: case 0xE5: case 0xF5: case 0xED:
        case 0xFD: case 0xF9: case 0xE1: case 0xF1:{
            SBC(state, state->readRAM(address));
            sprintf(instruction, "SBC");
            break;}

        //CPY
        case 0xC0: case 0xC4:
        case 0xCC:{
            int comparator = state->readRAM(address);
            updateCCompare(state, state->Y,comparator);
            updateZN(state, state->Y-comparator);
            sprintf(instruction, "CPY");
            break;}

        //CPX
        case 0xE0: case 0xE4:
        case 0xEC:{
            int comparator = state->readRAM(address);
            updateCCompare(state, state->X,comparator);
            updateZN(state, state->X-comparator);
            sprintf(instruction, "CPX");
            break;}

        /***********************
        ** STACK OPERATIONS
        ***********************/
        //PHP Implied
        case 0x08:{
            pushStack(state, state->P | (1<<UNDEFINED) | (1<<B));
            sprintf(instruction, "PHP");
            break;}

        //PLP Implied
        case 0x28:{
            state->P = ((popStack(state) & ~(1<<B)) | (1<<UNDEFINED));
            sprintf(instruction, "PLP");
            break;}

        //PHA Implied
        case 0x48:{
            pushStack(state, state->A);
            sprintf(instruction, "PHA");
            break;}

        //PLA Implied
        case 0x68:{
            state->A = popStack(state);
            updateZN(state, state->A);
            sprintf(instruction, "PLA");
            break;}

        /***********************
        ** REGISTER OPERATIONS
        ***********************/
        //SEI Implied
        case 0x78:{
            state->P |= (1<<I);
            sprintf(instruction, "SEI");
            break;}

        //CLI Implied
        case 0x58:{
            state->P &= ~(1<<I);
            sprintf(instruction, "CLI");
            break;}

        //SED Implied
        case 0xF8:{
            state->P |= (1<<D);
            sprintf(instruction, "SED");
            break;}

        //CLD Implied
        case 0xD8:{
            state->P &= ~(1<<D);
            sprintf(instruction, "CLD");
            break;}

        //SEC Implied
        case 0x38:{
            state->P |= (1<<C);
            sprintf(instruction, "SEC");
            break;}

        //CLC Implied
        case 0x18:{
            state->P &= ~(1<<C);
            sprintf(instruction, "CLC");
            break;}

        //CLV Implied
        case 0xB8:{
            state->P &= ~(1<<V);
            sprintf(instruction, "CLV");
            break;}

        //ASL Accumulator
        case 0x0A:{
            state->A = ASL(state, state->A);
            sprintf(instruction, "ASL");
            break;}

        //ASL
        case 0x1E: case 0x0E: case 0x16:
        case 0x06:{
            clockCycles += state->writeRAM(address, ASL(state, state->readRAM(address)));
            sprintf(instruction, "ASL");
            break;}

        //LSR Accumulator
        case 0x4A:{
            state->A = LSR(state, state->A);
            sprintf(instruction, "LSR");
            break;}

        //LSR
        case 0x46: case 0x56: case 0x4E:
        case 0x5E:{
            clockCycles += state->writeRAM(address, LSR(state, state->readRAM(address)));
            sprintf(instruction, "LSR");
            break;}

        //ROR Accumulator
        case 0x6A:{
            state->A = ROR(state, state->A);
            sprintf(instruction, "ROR");
            break;}

        //ROR
        case 0x66: case 0x76: case 0x6E:
        case 0x7E:{
            clockCycles += state->writeRAM(address, ROR(state, state->readRAM(address)));
            sprintf(instruction, "ROR");
            break;}

        //ROL Accumulator
        case 0x2A:{
            state->A = ROL(state, state->A);
            sprintf(instruction, "ROL");
            break;}

        //ROL
        case 0x26: case 0x2E: case 0x3E:
        case 0x36:{
            clockCycles += state->writeRAM(address, ROL(state, state->readRAM(address)));
            sprintf(instruction, "ROL");
            break;}

        /***********************
        ** MEMORY OPERATIONS
        ***********************/
        //STA
        case 0x85: case 0x95: case 0x8D: case 0x9D:
        case 0x99: case 0x81: case 0x91:{
            clockCycles += state->writeRAM(address, state->A);
            sprintf(instruction, "STA");
            break;}

        //INC
        case 0xE6: case 0xF6: case 0xEE:
        case 0xFE:{
            INC(state, address);
            sprintf(instruction, "INC");
            break;}

        //DEC
        case 0xC6: case 0xD6:
        case 0xCE: case 0xDE:{
            DEC(state, address);
            sprintf(instruction, "DEC");
            break;}

        /***********************
        ** BRANCHING
        ***********************/
        //BNE Relative
        case 0xD0:{
            BXX(state, address, Z, false);
            sprintf(instruction, "BNE");
            break;}

        //BCS Relative
        case 0xB0:{
            BXX(state, address, C, true);
            sprintf(instruction, "BCS");
            break;}

        //BEQ Relative
        case 0xF0:{
            BXX(state, address, Z, true);
            sprintf(instruction, "BEQ");
            break;}

        //BMI Relative
        case 0x30:{
            BXX(state, address, N, true);
            sprintf(instruction, "BMI");
            break;}

        //BCC Relative
        case 0x90:{
            BXX(state, address, C, false);
            sprintf(instruction, "BCC");
            break;}

        //BVS Relative
        case 0x70:{
            BXX(state, address, V, true);
            sprintf(instruction, "BVS");
            break;}

        //BVC Relative
        case 0x50:{
            BXX(state, address, V, false);
            sprintf(instruction, "BVC");
            break;}

        //BPL Relative
        case 0x10:{
            BXX(state, address, N, false);
            sprintf(instruction, "BPL");
            break;}

        /***********************
        ** JUMPS AND RECALLS
        ***********************/
        //JSR Absolute
        case 0x20:{
            pushStack(state, ((state->PC-1) & 0xFF00) >> 8);
            pushStack(state, (state->PC-1) & 0xFF);

            state->PC = address;
            sprintf(instruction, "JSR");
            break;}

        //JMP Absolute
        case 0x4C:{
            state->PC = address;
            sprintf(instruction, "JMP");
            break;}

        //JMP Indirect
        case 0x6C:{
            state->PC = address;
            sprintf(instruction, "JMP");
            break;}

        //RTS Implied
        case 0x60:{
            uint8_t low = popStack(state);
            uint8_t high = popStack(state);

            state->PC = ((high << 8) | low)+1;

            sprintf(instruction, "RTS");
            break;}

        //RTI Implied
        case 0x40:{
            state->P = (popStack(state) & ~(1<<B)) | (1<<UNDEFINED);
            int low = popStack(state);
            int high = popStack(state);
            state->PC = ((high << 8) | low);
            sprintf(instruction, "RTI");
            break;}

        //BRK Implied
        case 0x00:{
            pushStack(state, ((state->PC+1) & 0xFF00) >> 8);
            pushStack(state, (state->PC+1) & 0xFF);
            pushStack(state, state->P | (1<<UNDEFINED) | (1<<B));
            uint8_t low = state->readRAM(0xFFFE);
            uint8_t high = state->readRAM(0xFFFF);
            state->PC = (high << 8) | low;
            state->P |= (1<<B);
            sprintf(instruction, "BRK");
            break;}

        /***********************
        ** UNOFFICIAL OPCODES
        ***********************/

        //LAX (LDA -> TAX)
        case 0xA7: case 0xB7: case 0xAF: case 0xBF:
        case 0xA3: case 0xB3:{
            state->A = state->readRAM(address);
            state->X = state->A;
            updateZN(state, state->X);
            sprintf(instruction, "LAX");
            break;}

        //SAX
        case 0x87: case 0x97: case 0x8F: case 0x83:{
            clockCycles += state->writeRAM(address, state->A & state->X);
            sprintf(instruction, "SAX");
            break;}

        //DCP
        case 0xC7: case 0xD7: case 0xCF: case 0xDF:
        case 0xDB: case 0xC3: case 0xD3:{
            DEC(state, address);
            uint8_t comparator = state->readRAM(address);
            updateCCompare(state, state->A,comparator);
            updateZN(state, state->A-comparator);
            sprintf(instruction, "DCP");
            break;}

        //ISB
        case 0xE7: case 0xF7: case 0xEF: case 0xFF:
        case 0xFB: case 0xE3: case 0xF3:{
            INC(state, address);
            SBC(state, state->readRAM(address));
            sprintf(instruction, "ISB");
            break;}

        //SLO
        case 0x07: case 0x17: case 0x0F: case 0x1F:
        case 0x1B: case 0x03: case 0x13:{
            SLO(state, address);
            sprintf(instruction, "SLO");
            break;}

        //RLA
        case 0x27: case 0x37: case 0x2F: case 0x3F:
        case 0x3B: case 0x23: case 0x33:{
            RLA(state, address);
            sprintf(instruction, "RLA");
            break;}

        //SRE
        case 0x47: case 0x57: case 0x4F: case 0x5F:
        case 0x43: case 0x53: case 0x5B:{
            SRE(state, address);
            sprintf(instruction, "SRE");
            break;}

        //RRA
        case 0x77: case 0x6F: case 0x7F: case 0x7B:
        case 0x67: case 0x63: case 0x73:{
            RRA(state, address);
            sprintf(instruction, "RRA");
            break;}

        //NOP
        case 0x04: case 0x44: case 0x64: case 0x80: case 0x0C: case 0x14:
        case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4: case 0x1c:
        case 0x3c: case 0x5c: case 0x7c: case 0xdc: case 0xfc: case 0x1A:
        case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: case 0xEA:{
            sprintf(instruction, "NOP");
            break;}

        default:
            break;
    }

    //print debug info
    //sprintf(c1, "%02X", opcode);
    //printf("%04X  %s %s %s  %s %s  A:%02X X:%02X Y:%02X P:%02X SP:%02X \n", oldPC, c1, c2, c3, instruction, after, oldA, oldX, oldY, oldP, oldSP);
    return clockCycles;
}

uint8_t CPU6502state::writeRAM(uint16_t address, uint8_t value) {
    while(address >= 0x2008 && address < 0x4000) {
        address -= 8;
    }

    while(address >= 0x0800 && address < 0x2000) {
        address -= 0x0800;
    }

    // Deal with ppu
    if ((address >= 0x2000 && address <= 0x2007) || address == 0x4014) {
        ppu->writeRegisters(address, value);
    }

    // Controller 1
    else if(address == 0x4016) {
        writeController(&NES_Controller, value);
    }

    // Regular write
    else if (address < 0x800) {
        ram[address] = value;
    }

    return 0;
}

uint8_t CPU6502state::readRAM(uint16_t address) {
    while(address >= 0x2008 && address < 0x4000) {
        address -= 8; //handle mirroring
    }
    while(address >= 0x0800 && address < 0x2000) {
        address -= 0x0800; //handle mirroring
    }

    uint8_t retVal = 0; 

    // PPU registers
    if((address >= 0x2000 && address <= 0x2007) || address == 0x4014) {
        return ppu->readRegisters(address);
    }

    // Controller 1
    else if(address == 0x4016) {
        retVal = getNextButton(&NES_Controller);
    }

    // Regular read
    else if (address < 0x800) {
        retVal = this->ram[address];
    }

    // Cartridge
    else if (address >= 0x4020) {
        retVal = cartridge.readMemory(address);
    }

    return retVal;
}