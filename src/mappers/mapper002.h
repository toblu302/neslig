#include "mapper.h"

#include <iostream>

class Mapper002 : public Mapper {

    public:
    Mapper002() : Mapper() {
        this->mapper_id = "Mapper 002 (UNROM)";
    }

    uint8_t ReadPrg(const uint16_t &address) {
        if(address < 0x8000) {
            return 0;
        }

        if(0x8000 <= address && address <= 0xBFFF) {
            uint16_t idx = address - 0x8000;
            uint8_t retVal = prg_rom_banks.at(current_bank).at(idx);
            return retVal;
        }
        uint16_t idx = address-0xC000;
        uint8_t retval = prg_rom_banks.at(prg_rom_banks.size()-1).at(idx);
        return retval;
    };

    void WritePrg(const uint16_t &address, const uint8_t &value) {
        if(address >= 0x8000) {
            current_bank = value&0x7;
        }
    };

    uint8_t ReadChr(const uint16_t &address) {
        return chr_ram.at(address % 0x2000);
    }

    void WriteChr(const uint16_t &address, const uint8_t &value) {
        chr_ram[address % 0x2000] = value;
    }

    private:
        uint8_t current_bank = 0;
        std::array<uint8_t, 0x2000> chr_ram;
};