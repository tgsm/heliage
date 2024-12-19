#pragma once

#include <cstdint>
#include <type_traits>

typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

typedef std::int8_t s8;
typedef std::int16_t s16;
typedef std::int32_t s32;
typedef std::int64_t s64;

template <typename T>
concept EightBit = std::is_same_v<T, u8> || std::is_same_v<T, s8>;

template <typename T>
concept SixteenBit = std::is_same_v<T, u16> || std::is_same_v<T, s16>;
