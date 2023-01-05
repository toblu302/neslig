#ifndef FILEREADER_H_INCLUDED
#define FILEREADER_H_INCLUDED

#include <iostream>
#include <vector>
#include <array>

class Cartridge {
    public:
        uint8_t mapper;
        std::vector< std::array<uint8_t, 0x4000> > prg_rom_banks;
        std::vector< std::array<uint8_t, 0x2000> > chr_rom_banks;

        std::array<uint8_t, 0x10000> fake_ram;

        uint8_t readMemory(uint16_t address) {

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
};

Cartridge read_file(std::string filename);

#endif // FILEREADER_H_INCLUDED
