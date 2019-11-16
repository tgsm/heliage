#pragma once

#include <fstream>
#include <vector>
#include "types.h"

class Cartridge {
public:
    Cartridge(const std::string filename);

    std::string GetGameTitle(std::vector<char> rom);

    u8 Read8(u16 addr);
private:
    std::vector<char> ReadFileBytes(const std::string filename);
    std::vector<char> rom;
};
