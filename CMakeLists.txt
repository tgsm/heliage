cmake_minimum_required(VERSION 3.1)

set(PROJECT heliage)
project(${PROJECT})
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "-g -pg")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(SDL2 REQUIRED)

set(SOURCES
    src/frontend/sdl.cpp

    src/bootrom.cpp
    src/bus.cpp
    src/cartridge.cpp
    src/gb.cpp
    src/joypad.cpp
    src/main.cpp
    src/ppu.cpp
    src/sm83.cpp
    src/timer.cpp
)

set(HEADERS
    src/bootrom.h
    src/bus.h
    src/cartridge.h
    src/gb.h
    src/joypad.h
    src/logging.h
    src/ppu.h
    src/sm83.h
    src/timer.h
    src/types.h
)

add_compile_options(-Wall -Wextra)

add_executable(${PROJECT} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT} SDL2)
