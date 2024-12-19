#pragma once

#include "types.h"

namespace Common {

template <u8 Bit, typename T> requires (EightBit<T> || SixteenBit<T>)
constexpr bool IsBitSet(const T data) {
    return (data & (1 << Bit)) != 0;
}

template <u8... Bits, typename T> requires (EightBit<T> || SixteenBit<T>)
constexpr void SetBits(T& data) {
    data |= ((1 << Bits) | ...);
}

template <u8... Bits, typename T> requires (EightBit<T> || SixteenBit<T>)
constexpr void ResetBits(T& data) {
    data &= ~((1 << Bits) | ...);
}

}
