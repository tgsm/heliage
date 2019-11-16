cmake_minimum_required(VERSION 3.1)

set(PROJECT heliage)
project(${PROJECT})
set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    src/cartridge.cpp
    src/gb.cpp
    src/logging.cpp
    src/main.cpp
    src/mmu.cpp
    src/ppu.cpp
    src/sm83.cpp
)

set(HEADERS
    src/cartridge.h
    src/gb.h
    src/logging.h
    src/mmu.h
    src/ppu.h
    src/sm83.h
    src/types.h
)

add_compile_options(-Wall -Wextra)

add_executable(${PROJECT} ${SOURCES} ${HEADERS})
# target_link_libraries(${PROJECT})
