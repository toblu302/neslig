#ifndef MAPPER_H_INCLUDED
#define MAPPER_H_INCLUDED

#include <iostream>
#include <vector>
#include <array>
#include <cstdint>

class Mapper {
    public:
        virtual uint8_t ReadPrg(const uint16_t &address); 
        virtual void WritePrg(const uint16_t &address, const uint8_t &value) {};

        virtual uint8_t ReadChr(const uint16_t &address);
        virtual void WriteChr(const uint16_t &address, const uint8_t &value) {};

        virtual void AddPrgRomBank(std::array<uint8_t, 0x4000> prg_bank);
        virtual void AddChrRomBank(std::array<uint8_t, 0x2000> chr_bank);

        std::array<uint8_t, 0x10000> fake_ram;

        friend std::ostream& operator<<(std::ostream& os, const Mapper& mapper);

    protected:
        std::vector< std::array<uint8_t, 0x4000> > prg_rom_banks;
        std::vector< std::array<uint8_t, 0x2000> > chr_rom_banks;

        std::string mapper_id = "Mapper 000 (NROM)";
};

#endif // MAPPER_H_INCLUDED