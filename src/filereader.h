#ifndef FILEREADER_H_INCLUDED
#define FILEREADER_H_INCLUDED

#include <iostream>
#include <vector>
#include <array>

struct Cartridge {
    uint8_t mapper;
    std::vector< std::array<uint8_t, 0x4000> > prg_rom_banks;
    std::vector< std::array<uint8_t, 0x2000> > chr_rom_banks;
};

Cartridge read_file(std::string filename);

#endif // FILEREADER_H_INCLUDED
