#include "mapper.h"

uint8_t Mapper::ReadPrg(const uint16_t &address) {
    if(address >= 0x8000) {
        uint16_t idx = address - 0x8000;
        uint8_t rom_bank = 0;

        if(address >= 0xc000 && prg_rom_banks.size() == 2) {
            rom_bank = 1;
        }
        if(idx >= 0x4000) {
            idx -= 0x4000;
        }

        uint8_t retVal = prg_rom_banks.at(rom_bank).at(idx);
        return retVal;
    }
    else {
        return fake_ram.at(address);
    }
}

uint8_t Mapper::ReadChr(const uint16_t &address) {
    return chr_rom_banks.at(0).at(address);
}

void Mapper::AddPrgRomBank(std::array<uint8_t, 0x4000> prg_bank) {
    this->prg_rom_banks.push_back(prg_bank);
}

void Mapper::AddChrRomBank(std::array<uint8_t, 0x2000> chr_bank) {
    this->chr_rom_banks.push_back(chr_bank);
}

std::ostream& operator<<(std::ostream& stream, const Mapper& mapper)
{
    stream << mapper.mapper_id << " PRGROM: " << mapper.prg_rom_banks.size() << " CHRROM: " << mapper.chr_rom_banks.size() << std::endl;
    return stream;
}