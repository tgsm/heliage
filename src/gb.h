#pragma once

#include "mmu.h"
#include "sm83.h"

class GB {
public:
    GB(Cartridge cartridge);

    void Run();
private:
    MMU mmu;
    SM83 sm83;
};