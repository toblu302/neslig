#include "cpu6502.h"
#include "MMU.h"

/******************
* Addressing modes
******************/
uint16_t addressImmediate(CPU6502state* state) {
    return state->PC++;
}

uint16_t addressZeroPage(CPU6502state* state) {
    return readRAM(state->PC++);
}

uint16_t addressZeroPageX(CPU6502state* state) {
    return (readRAM(state->PC++) + state->X) & 0xFF;
}

uint16_t addressZeroPageY(CPU6502state* state) {
    return (readRAM( state->PC++ )+ state->Y) & 0xFF;
}

//returns an "8 bit" offset
int8_t addressRelative(CPU6502state* state) {
    return readRAM( state->PC++ );
}

uint16_t addressAbsolute(CPU6502state* state) {
    uint8_t low = readRAM(state->PC++);
    uint8_t high = readRAM(state->PC++);
    return (high << 8) | low;
}

uint16_t addressAbsoluteX(CPU6502state* state) {
    uint8_t low = readRAM(state->PC++);
    uint8_t high = readRAM(state->PC++);
    return ( ((high << 8) | low) + state->X);
}

uint16_t addressAbsoluteY(CPU6502state* state) {
    uint8_t low = readRAM(state->PC++);
    uint8_t high = readRAM(state->PC++);
    return ( ((high << 8) | low) + state->Y);
}

uint16_t addressIndirect(CPU6502state* state) {
    uint8_t low = readRAM(state->PC++);
    uint8_t high = readRAM(state->PC++);
    uint16_t target = (high << 8) | low;

    uint8_t targetLow = readRAM( ((high << 8) | low) );
    uint8_t targetHigh = readRAM( (((high << 8) | low) + 1) );
    if( ((((high << 8) | low) + 1) & 0xFF) == 0) {
        targetHigh = readRAM( (((high << 8) | low) + 1) - 0x100 );
    }

    return (targetHigh << 8) | targetLow;
}

uint16_t addressIndexedIndirect(CPU6502state* state) {
    int address = readRAM(state->PC++);

    int low = readRAM((address+state->X)&0xFF);
    int high = readRAM((address+state->X+1)&0xFF);

    return (high << 8) | low;
}

uint16_t addressIndirectIndexed(CPU6502state* state) {
    uint32_t address = readRAM(state->PC++);

    uint32_t low = readRAM(address++);
    uint32_t high = readRAM( address & 0xFF );
    return (((high << 8) | low) + state->Y);
}


uint16_t getAddress(CPU6502state *state, uint8_t opcode) {
    enum addressingMode mode = addressModeLookup[opcode];
    switch(mode) {
        case IMM:
            return addressImmediate(state);
        case ZER:
            return addressZeroPage(state);
        case ZERX:
            return addressZeroPageX(state);
        case ZERY:
            return addressZeroPageY(state);
        case REL:
            return state->PC + addressRelative(state) + 1;
        case ABS:
            return addressAbsolute(state);
        case ABSX:
            return addressAbsoluteX(state);
        case ABSY:
            return addressAbsoluteY(state);
        case IND:
            return addressIndirect(state);
        case INXIND:
            return addressIndexedIndirect(state);
        case INDINX:
            return addressIndirectIndexed(state);
        case NONE:
        case IMP:
        case ACC:
            return 0;
    }
}

/******************
* Instructions
******************/
void ADC(CPU6502state* state, uint8_t argument) {
    uint16_t result = state->A + argument + (state->P&(1<<C)>>C);
    state->P &= ~(1<<C);
    if(result & 0xFF00) {
        state->P |= (1<<C);
    }
    state->P &= ~(1<<V);
    if (~(state->A ^ argument) & (state->A ^ result) & 0x80){
        state->P |= (1<<V);
    }
    state->A = result & 0xFF;
    updateZN(state, state->A);
}

void SBC(CPU6502state* state, uint8_t argument) {
    ADC(state, ~argument);
}

uint8_t LSR(CPU6502state* state, uint8_t value) {
    uint8_t returnValue = value;
    state->P &= ~(1<<C);
    if(value & 0x1) {
        state->P |= (1<<C);
    }
    returnValue = (returnValue >> 1);
    returnValue &= ~(1<<7);
    updateZN(state, returnValue);
    return returnValue;
}

uint8_t ASL(CPU6502state* state, uint8_t value) {
    uint8_t returnValue = value;
    state->P &= ~(1<<C);
    if(value & 0x80) {
        state->P |= (1<<C);
    }
    returnValue = (returnValue << 1);
    returnValue &= ~1;
    updateZN(state, returnValue);
    return returnValue;
}

uint8_t ROR(CPU6502state* state, uint8_t value) {
    uint8_t returnValue = value;
    returnValue = (returnValue >> 1);

    if(state->P & (1<<C)) {
        returnValue |= (1<<7);
    }
    state->P &= ~(1<<C);
    if(value & 0x1) {
        state->P |= (1<<C);
    }
    updateZN(state, returnValue);
    return returnValue;
}

uint8_t ROL(CPU6502state* state, uint8_t value) {
    uint8_t returnValue = value;
    returnValue = (returnValue << 1);

    if(state->P & (1<<C)) {
        returnValue |= 1;
    }
    state->P &= ~(1<<C);
    if(value & 0x80) {
        state->P |= (1<<C);
    }
    updateZN(state, returnValue);
    return returnValue;
}

void DEC(CPU6502state* state, uint16_t address) {
    writeRAM(address, readRAM(address)-1);
    updateZN(state, readRAM(address));
}

void INC(CPU6502state* state, uint16_t address) {
    writeRAM(address, readRAM(address)+1);
    updateZN(state, readRAM(address));
}

void SLO(CPU6502state* state, uint16_t address) {
    writeRAM(address, ASL(state, readRAM(address)));
    state->A |= readRAM(address);
    updateZN(state, state->A);
}

void RLA(CPU6502state* state, uint16_t address) {
    writeRAM(address, ROL(state, readRAM(address)));
    state->A &= readRAM(address);
    updateZN(state, state->A);
}

void RRA(CPU6502state* state, uint16_t address) {
    writeRAM(address, ROR(state, readRAM(address)));
    ADC(state, readRAM(address));
}

void SRE(CPU6502state* state, uint16_t address) {
    writeRAM(address, LSR(state, readRAM(address)));

    state->A ^= readRAM(address);
    updateZN(state, state->A);
}

uint8_t BXX(CPU6502state* state, uint16_t address, int flag, bool shouldBeSet) {
    uint8_t clockCycles = 0;
    bool isSet = ((state->P & (1<<flag)) != 0x00);
    if( isSet == shouldBeSet ) {
        clockCycles += 1;
        if((address & 0xFF00) != (state->PC & 0xFF00)) {
            clockCycles += 1;
        }
        state->PC = address;
    }
    return clockCycles;
}
