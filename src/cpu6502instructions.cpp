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
    Tick();
    return (ReadRam(PC++) + X) & 0xFF;
}

uint16_t CPU6502state::addressZeroPageY() {
    Tick();
    return (ReadRam( PC++ )+ Y) & 0xFF;
}

uint16_t CPU6502state::addressRelative() {
    uint16_t address = PC + static_cast<int8_t>(ReadRam(PC)) + 1;
    PC += 1;
    return address;
}

uint16_t CPU6502state::addressAbsolute() {
    uint8_t low = ReadRam(PC++);
    uint8_t high = ReadRam(PC++);
    return (high << 8) | low;
}

uint16_t CPU6502state::addressAbsoluteX() {
    uint8_t low = ReadRam(PC++);
    uint8_t high = ReadRam(PC++);
    uint16_t new_address = ((high << 8) | low) + X;
    if(CrossedPage(new_address, X)) {
        Tick();
    }
    return new_address;
}

uint16_t CPU6502state::addressAbsoluteY() {
    uint8_t low = ReadRam(PC++);
    uint8_t high = ReadRam(PC++);
    uint16_t new_address = ((high << 8) | low) + Y;
    if(CrossedPage(new_address, Y)) {
        Tick();
    }
    return new_address;
}

uint16_t CPU6502state::addressIndirect() {
    uint8_t low = ReadRam(PC++);
    uint8_t high = ReadRam(PC++);
    uint16_t target = (high << 8) | low;

    uint8_t targetLow = ReadRam(target);
    uint16_t next_target = target+1;
    if( ((((high << 8) | low) + 1) & 0xFF) == 0) {
        next_target -= 0x100;
    }
    uint8_t targetHigh = ReadRam(next_target);

    return (targetHigh << 8) | targetLow;
}

uint16_t CPU6502state::addressIndexedIndirect() {
    Tick();

    int address = ReadRam(PC++);

    int low = ReadRam((address+X)&0xFF);
    int high = ReadRam((address+X+1)&0xFF);

    return (high << 8) | low;
}

uint16_t CPU6502state::addressIndirectIndexed() {
    uint32_t address = ReadRam(PC++);

    uint32_t low = ReadRam(address++);
    uint32_t high = ReadRam( address & 0xFF );
    uint16_t new_address = (((high << 8) | low) + Y);
    if(CrossedPage(new_address, Y)) {
        Tick();
    }
    return new_address;
}

/******************
* Instructions
******************/
void CPU6502state::ADC(const uint16_t &address) {
    uint8_t argument = ReadRam(address);
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

void CPU6502state::AND(const uint16_t &address) {
    A &= ReadRam(address);
    updateZN(A);
}

void CPU6502state::ASL(const uint16_t &address) {
    uint8_t value = ReadRam(address);
    Tick();
    uint8_t rotatedValue = LeftShift(value);
    WriteRam(address, rotatedValue);
}

void CPU6502state::BIT(const uint16_t &address) {
    uint8_t inMemory = ReadRam(address);
    updateZN(inMemory&A);
    P &= ~(1<<V);
    if(inMemory & (1 << 6)) P |= (1<<V);
    P &= ~(1<<N);
    if(inMemory & (1 << 7)) P |= (1<<N);
}

void CPU6502state::Branch(const uint16_t &address, const Flags &flag, bool shouldBeSet) {
    bool isSet = ((P & (1<<flag)) != 0x00);
    if( isSet == shouldBeSet ) {
        Tick();
        if((PC&0xFF00) != (address&0xFF00)) {
            Tick();
        }
        PC = address;
    }
}

void CPU6502state::BRK() {
    pushStack(((PC+1) & 0xFF00) >> 8);
    pushStack((PC+1) & 0xFF);
    pushStack(P | (1<<UNDEFINED) | (1<<B));
    uint8_t low = ReadRam(0xFFFE);
    uint8_t high = ReadRam(0xFFFF);
    PC = (high << 8) | low;
    P |= (1<<B);
}

void CPU6502state::CL(const Flags &flag) {
    P &= ~(1 << flag);
    Tick();
}

void CPU6502state::Compare(uint8_t &reg, const uint16_t &address) {
    int comparator = ReadRam(address);
    updateCCompare(reg, comparator);
    updateZN(reg-comparator);
}

void CPU6502state::DCP(const uint16_t &address) {
    WriteRam(address, ReadRam(address)-1);
    uint8_t comparator = ReadRam(address);
    updateCCompare(A, comparator);
    updateZN(A-comparator);
}

void CPU6502state::DEC(const uint16_t &address) {
    WriteRam(address, ReadRam(address)-1);
    updateZN(ReadRam(address));
}

void CPU6502state::EOR(const uint16_t &address) {
    A ^= ReadRam(address);
    updateZN(A);
}

void CPU6502state::INC(const uint16_t &address) {
    WriteRam(address, ReadRam(address)+1);
    updateZN(ReadRam(address));
}

void CPU6502state::ISB(const uint16_t &address) {
    WriteRam(address, ReadRam(address)+1);
    SBC(address);
}

void CPU6502state::JMP(const uint16_t &address) {
    PC = address;
}

void CPU6502state::JSR(const uint16_t &address) {
    Tick();
    pushStack(((PC-1) & 0xFF00) >> 8);
    pushStack((PC-1) & 0xFF);

    PC = address;
}

void CPU6502state::LAX(const uint16_t &address) {
    A = ReadRam(address);
    X = A;
    updateZN(X);
}

void CPU6502state::LSR(const uint16_t &address) {
    uint8_t value = ReadRam(address);
    Tick();
    uint8_t rotatedValue = RightShift(value);
    WriteRam(address, rotatedValue);
}

void CPU6502state::LD(uint8_t &reg, const uint16_t &address) {
    reg = ReadRam(address);
    updateZN(reg);
}

void CPU6502state::ORA(const uint16_t &address) {
    A |= ReadRam(address);
    updateZN(A);
}

void CPU6502state::RLA(const uint16_t &address) {
    WriteRam(address, LeftRotate(ReadRam(address)));
    A &= ReadRam(address);
    updateZN(A);
}

void CPU6502state::ROL(const uint16_t &address) {
    uint8_t value = ReadRam(address);
    Tick();
    uint8_t rotatedValue = LeftRotate(value);
    WriteRam(address, rotatedValue);
}

void CPU6502state::ROR(const uint16_t &address) {
    uint8_t value = ReadRam(address);
    Tick();
    uint8_t rotatedValue = RightRotate(value);
    WriteRam(address, rotatedValue);
}

void CPU6502state::RRA(const uint16_t &address) {
    WriteRam(address, RightRotate(ReadRam(address)));
    ADC(address);
}

void CPU6502state::RTI() {
    Tick();
    Tick();
    P = (popStack() & ~(1<<B)) | (1<<UNDEFINED);
    int low = popStack();
    int high = popStack();
    PC = ((high << 8) | low);
}

void CPU6502state::RTS() {
    Tick();
    Tick();
    uint8_t low = popStack();
    uint8_t high = popStack();
    PC = ((high << 8) | low)+1;
    Tick();
}

void CPU6502state::SBC(const uint16_t &address) {
    uint8_t argument = ~ReadRam(address);
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

void CPU6502state::SE(const Flags &flag) {
    P |= (1 << flag);
    Tick();
}

void CPU6502state::SLO(const uint16_t &address) {
    WriteRam(address, LeftShift(ReadRam(address)));
    A |= ReadRam(address);
    updateZN(A);
}

void CPU6502state::SRE(const uint16_t &address) {
    WriteRam(address, RightShift(ReadRam(address)));

    A ^= ReadRam(address);
    updateZN(A);
}

void CPU6502state::ST(uint8_t &reg, const uint16_t &address) {
    WriteRam(address, reg);
}


/******************
* Utils
******************/
uint8_t CPU6502state::RightShift(const uint8_t &value) {
    P &= ~(1<<C);
    if(value & 0x1) {
        P |= (1<<C);
    }
    uint8_t returnValue = value;
    returnValue = (returnValue >> 1);
    returnValue &= ~(1<<7);
    updateZN(returnValue);
    return returnValue;
}

uint8_t CPU6502state::LeftShift(const uint8_t &value) {
    P &= ~(1<<C);
    if(value & 0x80) {
        P |= (1<<C);
    }
    uint8_t returnValue = value;
    returnValue = (returnValue << 1);
    returnValue &= ~1;
    updateZN(returnValue);
    return returnValue;
}

uint8_t CPU6502state::RightRotate(const uint8_t &value) {
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

uint8_t CPU6502state::LeftRotate(const uint8_t &value) {
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

bool CPU6502state::CrossedPage(const uint16_t &address, const uint8_t &increment) {
    return ((address-increment)&0xFF00) != (address&0xFF00);
}