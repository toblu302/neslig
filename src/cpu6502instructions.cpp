#include "cpu6502.h"

/******************
* Addressing modes
******************/
uint16_t CPU6502state::addressImmediate() {
    return PC++;
}

uint16_t CPU6502state::addressZeroPage() {
    return ReadRam(PC++);
}

uint16_t CPU6502state::addressZeroPageX() {
    return (ReadRam(PC++) + X) & 0xFF;
}

uint16_t CPU6502state::addressZeroPageY() {
    return (ReadRam( PC++ )+ Y) & 0xFF;
}

//returns an "8 bit" offset
int8_t CPU6502state::addressRelative() {
    return ReadRam( PC++ );
}

uint16_t CPU6502state::addressAbsolute() {
    uint8_t low = ReadRam(PC++);
    uint8_t high = ReadRam(PC++);
    return (high << 8) | low;
}

uint16_t CPU6502state::addressAbsoluteX() {
    uint8_t low = ReadRam(PC++);
    uint8_t high = ReadRam(PC++);
    return ( ((high << 8) | low) + X);
}

uint16_t CPU6502state::addressAbsoluteY() {
    uint8_t low = ReadRam(PC++);
    uint8_t high = ReadRam(PC++);
    return ( ((high << 8) | low) + Y);
}

uint16_t CPU6502state::addressIndirect() {
    uint8_t low = ReadRam(PC++);
    uint8_t high = ReadRam(PC++);
    uint16_t target = (high << 8) | low;

    uint8_t targetLow = ReadRam(target);
    uint8_t targetHigh = ReadRam(target+1);
    if( ((((high << 8) | low) + 1) & 0xFF) == 0) {
        targetHigh = ReadRam( (target+1) - 0x100 );
    }

    return (targetHigh << 8) | targetLow;
}

uint16_t CPU6502state::addressIndexedIndirect() {
    int address = ReadRam(PC++);

    int low = ReadRam((address+X)&0xFF);
    int high = ReadRam((address+X+1)&0xFF);

    return (high << 8) | low;
}

uint16_t CPU6502state::addressIndirectIndexed() {
    uint32_t address = ReadRam(PC++);

    uint32_t low = ReadRam(address++);
    uint32_t high = ReadRam( address & 0xFF );
    return (((high << 8) | low) + Y);
}


uint16_t CPU6502state::getAddress(uint8_t opcode) {
    enum addressingMode mode = addressModeLookup[opcode];
    switch(mode) {
        case IMM:
            return addressImmediate();
        case ZER:
            return addressZeroPage();
        case ZERX:
            return addressZeroPageX();
        case ZERY:
            return addressZeroPageY();
        case REL:
            return PC + addressRelative() + 1;
        case ABS:
            return addressAbsolute();
        case ABSX:
            return addressAbsoluteX();
        case ABSY:
            return addressAbsoluteY();
        case IND:
            return addressIndirect();
        case INXIND:
            return addressIndexedIndirect();
        case INDINX:
            return addressIndirectIndexed();
        case NONE:
        case IMP:
        case ACC:
            return 0;
    }
    return 0;
}

/******************
* Instructions
******************/
void CPU6502state::ADC(uint8_t argument) {
    uint16_t result = A + argument + (P&(1<<C)>>C);
    P &= ~(1<<C);
    if(result & 0xFF00) {
        P |= (1<<C);
    }
    P &= ~(1<<V);
    if (~(A ^ argument) & (A ^ result) & 0x80){
        P |= (1<<V);
    }
    A = result & 0xFF;
    updateZN(A);
}

void CPU6502state::SBC(uint8_t argument) {
    ADC(~argument);
}

uint8_t CPU6502state::LSR(uint8_t value) {
    uint8_t returnValue = value;
    P &= ~(1<<C);
    if(value & 0x1) {
        P |= (1<<C);
    }
    returnValue = (returnValue >> 1);
    returnValue &= ~(1<<7);
    updateZN(returnValue);
    return returnValue;
}

uint8_t CPU6502state::ASL(uint8_t value) {
    uint8_t returnValue = value;
    P &= ~(1<<C);
    if(value & 0x80) {
        P |= (1<<C);
    }
    returnValue = (returnValue << 1);
    returnValue &= ~1;
    updateZN(returnValue);
    return returnValue;
}

uint8_t CPU6502state::ROR(uint8_t value) {
    uint8_t returnValue = value;
    returnValue = (returnValue >> 1);

    if(P & (1<<C)) {
        returnValue |= (1<<7);
    }
    P &= ~(1<<C);
    if(value & 0x1) {
        P |= (1<<C);
    }
    updateZN(returnValue);
    return returnValue;
}

uint8_t CPU6502state::ROL(uint8_t value) {
    uint8_t returnValue = value;
    returnValue = (returnValue << 1);

    if(P & (1<<C)) {
        returnValue |= 1;
    }
    P &= ~(1<<C);
    if(value & 0x80) {
        P |= (1<<C);
    }
    updateZN(returnValue);
    return returnValue;
}

void CPU6502state::DEC(uint16_t address) {
    WriteRam(address, ReadRam(address)-1);
    updateZN(ReadRam(address));
}

void CPU6502state::INC(uint16_t address) {
    WriteRam(address, ReadRam(address)+1);
    updateZN(ReadRam(address));
}

void CPU6502state::SLO(uint16_t address) {
    WriteRam(address, ASL(ReadRam(address)));
    A |= ReadRam(address);
    updateZN(A);
}

void CPU6502state::RLA(uint16_t address) {
    WriteRam(address, ROL(ReadRam(address)));
    A &= ReadRam(address);
    updateZN(A);
}

void CPU6502state::RRA(uint16_t address) {
    WriteRam(address, ROR(ReadRam(address)));
    ADC(ReadRam(address));
}

void CPU6502state::SRE(uint16_t address) {
    WriteRam(address, LSR(ReadRam(address)));

    A ^= ReadRam(address);
    updateZN(A);
}

uint8_t CPU6502state::BXX(uint16_t address, int flag, bool shouldBeSet) {
    uint8_t clockCycles = 0;
    bool isSet = ((P & (1<<flag)) != 0x00);
    if( isSet == shouldBeSet ) {
        clockCycles += 1;
        if((address & 0xFF00) != (PC & 0xFF00)) {
            clockCycles += 1;
        }
        PC = address;
    }
    return clockCycles;
}
