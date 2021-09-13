#pragma once

#include <cassert>
#include <fmt/core.h>
#include <fmt/color.h>

#define LTRACE(format, ...) fmt::print("AF={:04X} BC={:04X} DE={:04X} HL={:04X} SP={:04X} PC={:04X}  " format "\n", af, bc, de, hl, sp, pc_at_opcode, ##__VA_ARGS__)
#define LDEBUG(format, ...) fmt::print(fg(fmt::color::teal), "debug: " format "\033[0m\n", ##__VA_ARGS__)
#define LINFO(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::white), "info: " format "\033[0m\n", ##__VA_ARGS__)
#define LWARN(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::yellow), "warning: " format "\033[0m\n", ##__VA_ARGS__)
#define LERROR(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::red), "error: " format "\033[0m\n", ##__VA_ARGS__)
#define LFATAL(format, ...) fmt::print(fmt::emphasis::bold | fg(fmt::color::fuchsia), "fatal: " format "\033[0m\n", ##__VA_ARGS__)

#define UNREACHABLE() \
    LFATAL("unreachable code at {}:{}", __FILE__, __LINE__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define UNREACHABLE_MSG(format, ...) \
    LFATAL("unreachable code at {}:{}", __FILE__, __LINE__); \
    LFATAL(format, ##__VA_ARGS__); \
    assert(false); \
    std::exit(1); // exit just in case assert doesn't

#define ASSERT(cond) \
    if (!(cond)) { \
        LFATAL("assertion failed at {}:{}", __FILE__, __LINE__); \
        LFATAL("{}", #cond); \
        assert(cond); \
        std::exit(1); \
    }

#define ASSERT_MSG(cond, format, ...) \
    if (!(cond)) { \
        LFATAL("assertion failed at {}:{}", __FILE__, __LINE__); \
        LFATAL(format, ##__VA_ARGS__); \
        assert(cond); \
        std::exit(1); \
    }
