cmake_minimum_required(VERSION 3.1)

project(heliage CXX)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "-g -pg")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(SDL2 REQUIRED)

set(HELIAGE_FRONTEND "SDL2" CACHE STRING "The frontend heliage will run on")
set_property(CACHE HELIAGE_FRONTEND PROPERTY STRINGS SDL2)
if (${HELIAGE_FRONTEND} MATCHES "SDL2")
    add_compile_definitions("HELIAGE_FRONTEND_SDL")
endif()

set(SOURCES
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

if (${HELIAGE_FRONTEND} MATCHES "SDL2")
    set(SOURCES ${SOURCES} src/frontend/sdl.cpp)
endif()

add_compile_options(-Wall -Wextra)

add_executable(heliage)
target_sources(heliage PRIVATE ${SOURCES} ${HEADERS})
target_link_libraries(heliage SDL2 pthread)
