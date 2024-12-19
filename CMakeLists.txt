cmake_minimum_required(VERSION 3.10)

project(heliage CXX)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_DEBUG "-g -pg")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

option(HELIAGE_PRINT_SERIAL_BYTES "If enabled, any bytes sent to serial (0xFF01) will be printed to stdout" OFF)

set(HELIAGE_FRONTEND "SDL2" CACHE STRING "The frontend heliage will run on")
set_property(CACHE HELIAGE_FRONTEND PROPERTY STRINGS SDL2 ImGui Null)
if (${HELIAGE_FRONTEND} MATCHES "SDL2")
    add_compile_definitions("HELIAGE_FRONTEND_SDL")
    find_package(SDL2 REQUIRED)
elseif (${HELIAGE_FRONTEND} MATCHES "ImGui")
    add_compile_definitions("HELIAGE_FRONTEND_IMGUI")
    find_package(SDL2 REQUIRED)
endif()

if (${HELIAGE_PRINT_SERIAL_BYTES})
    add_compile_definitions("HELIAGE_PRINT_SERIAL_BYTES")
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
    src/common/bits.h
    src/common/types.h
    src/bootrom.h
    src/bus.h
    src/cartridge.h
    src/gb.h
    src/joypad.h
    src/logging.h
    src/ppu.h
    src/sm83.h
    src/timer.h
)

if (${HELIAGE_FRONTEND} MATCHES "SDL2")
    set(SOURCES ${SOURCES} src/frontend/sdl.cpp)
    set(HEADERS ${HEADERS} src/frontend/sdl.h)
elseif (${HELIAGE_FRONTEND} MATCHES "ImGui")
    set(SOURCES ${SOURCES}
        src/frontend/imgui.cpp

        dependencies/imgui/examples/imgui_impl_opengl2.cpp
        dependencies/imgui/examples/imgui_impl_sdl.cpp
        dependencies/imgui/imgui.cpp
        dependencies/imgui/imgui_demo.cpp
        dependencies/imgui/imgui_draw.cpp
        dependencies/imgui/imgui_widgets.cpp
    )
elseif (${HELIAGE_FRONTEND} MATCHES "Null")
    set(SOURCES ${SOURCES} src/frontend/null.cpp)
    set(HEADERS ${HEADERS} src/frontend/null.h)
endif()

add_compile_options(-Wall -Wextra)

add_subdirectory(dependencies/fmt)

add_executable(heliage)
target_sources(heliage PRIVATE ${SOURCES} ${HEADERS})
target_include_directories(heliage PRIVATE dependencies)
target_link_libraries(heliage fmt)

if (${HELIAGE_FRONTEND} MATCHES "SDL2")
    target_link_libraries(heliage SDL2)
elseif (${HELIAGE_FRONTEND} MATCHES "ImGui")
    find_path(SDL2_INCLUDE_DIR NAMES SDL_opengl.h PATH_SUFFIXES SDL2)
    target_include_directories(heliage PRIVATE dependencies/imgui ${SDL2_INCLUDE_DIR})
    target_link_libraries(heliage SDL2 GL pthread)
endif()
