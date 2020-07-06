cmake_minimum_required(VERSION 3.1)

project(heliage CXX)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "-g -pg")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(SDL2 REQUIRED)

set(HELIAGE_FRONTEND "SDL2" CACHE STRING "The frontend heliage will run on")
set_property(CACHE HELIAGE_FRONTEND PROPERTY STRINGS SDL2 ImGui Null)
if (${HELIAGE_FRONTEND} MATCHES "SDL2")
    add_compile_definitions("HELIAGE_FRONTEND_SDL")
elseif (${HELIAGE_FRONTEND} MATCHES "ImGui")
    add_compile_definitions("HELIAGE_FRONTEND_IMGUI")
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
    set(HEADERS ${HEADERS} src/frontend/sdl.h)
elseif (${HELIAGE_FRONTEND} MATCHES "ImGui")
    set(SOURCES ${SOURCES}
        src/frontend/imgui.cpp

        imgui/examples/imgui_impl_opengl2.cpp
        imgui/examples/imgui_impl_sdl.cpp
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
    )
elseif (${HELIAGE_FRONTEND} MATCHES "Null")
    set(SOURCES ${SOURCES} src/frontend/null.cpp)
    set(HEADERS ${HEADERS} src/frontend/null.h)
endif()

add_compile_options(-Wall -Wextra)

add_executable(heliage)
target_sources(heliage PRIVATE ${SOURCES} ${HEADERS})
target_link_libraries(heliage SDL2 pthread)

if (${HELIAGE_FRONTEND} MATCHES "ImGui")
    find_path(SDL2_INCLUDE_DIR NAMES SDL_opengl.h PATH_SUFFIXES SDL2)
    target_include_directories(heliage PRIVATE imgui . ${SDL2_INCLUDE_DIR})
    target_link_libraries(heliage GL)
endif()
