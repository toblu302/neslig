#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <vector>

#include "filereader.h"

#include "mappers/mapper002.h"

std::shared_ptr<Mapper> read_file(std::string filename) {
    std::ifstream filestream(filename, std::ios_base::binary);

    if(!filestream) {
        std::cerr << "Error: Could not load the file " << filename << std::endl;
    }

    std::vector<char> raw_rom(
         (std::istreambuf_iterator<char>(filestream)),
         (std::istreambuf_iterator<char>()));

    filestream.close();

    if(raw_rom.size() < 16) {
        std::cerr << "iNES file is too small" << std::endl;
    }

    // Read constant header
    std::string constant(raw_rom.begin(), raw_rom.begin()+4);
    if(constant != "NES\x1a") {
        std::cerr << "Error: File is not an iNES-file." << std::endl;
    }

    uint8_t num_prg_rom = raw_rom.at(4);
    uint8_t num_chr_rom = raw_rom.at(5);
    uint8_t flags6 = raw_rom.at(6);
    uint8_t flags7 = raw_rom.at(7);

    uint8_t mapper_id = ( (flags7 & 0xF0) | ((flags6 & 0xF0) >> 4));

    std::shared_ptr<Mapper> mapper;
    switch(mapper_id) {
        case 0: mapper = std::make_shared<Mapper>(); break;
        case 2: mapper = std::make_shared<Mapper002>(); break;
    }

    size_t prg_rom_offset = 0x10;
    for(size_t i=0; i<num_prg_rom; ++i) {
        std::array<uint8_t, 0x4000> prg_rom;
        std::copy_n(raw_rom.begin()+prg_rom_offset+0x4000*i, 0x4000, prg_rom.begin());
        mapper->AddPrgRomBank(prg_rom);
    }

    size_t chr_rom_offset = prg_rom_offset+0x4000*num_prg_rom;
    for(size_t i=0; i<num_chr_rom; ++i) {
        std::array<uint8_t, 0x2000> chr_rom;
        std::copy_n(raw_rom.begin()+chr_rom_offset+0x2000*i, 0x2000, chr_rom.begin());
        mapper->AddChrRomBank(chr_rom);
    }

    return mapper;
}