#pragma once

#include <hmpc/assert.hpp>

#include <bit>

namespace hmpc::detail
{
    constexpr bool implies(bool a, bool b) HMPC_NOEXCEPT
    {
        return a <= b;
    }

    template<typename T>
    constexpr T div_ceil(T numerator, T denominator) HMPC_NOEXCEPT
    {
        return (numerator + denominator - static_cast<T>(1)) / denominator;
    }

    constexpr hmpc::size bit_width(hmpc::size value) HMPC_HOST_NOEXCEPT
    {
        if constexpr (std::numeric_limits<hmpc::size>::is_signed)
        {
            HMPC_HOST_ASSERT(value >= 0);
            return std::bit_width(static_cast<std::make_unsigned_t<hmpc::size>>(value));
        }
        else
        {
            return std::bit_width(value);
        }
    }

    constexpr bool has_single_bit(hmpc::size value) HMPC_HOST_NOEXCEPT
    {
        if constexpr (std::numeric_limits<hmpc::size>::is_signed)
        {
            HMPC_HOST_ASSERT(value >= 0);
            return std::has_single_bit(static_cast<std::make_unsigned_t<hmpc::size>>(value));
        }
        else
        {
            return std::has_single_bit(value);
        }
    }

    constexpr hmpc::size countr_zero(hmpc::size value) HMPC_HOST_NOEXCEPT
    {
        if constexpr (std::numeric_limits<hmpc::size>::is_signed)
        {
            HMPC_HOST_ASSERT(value >= 0);
            return std::countr_zero(static_cast<std::make_unsigned_t<hmpc::size>>(value));
        }
        else
        {
            return std::countr_zero(value);
        }
    }

    /// #### Algorithm reference
    /// - [1] Sean Eron Anderson: "Bit Twiddling Hacks: Reversing bit sequences: Reverse an N-bit quantity in parallel with 5 * lg(N) operations." Online, 2005. [Link](http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel), accessed 2023-09-01.
    constexpr std::uint32_t bit_reverse(std::uint32_t value, hmpc::size_constant<2>) HMPC_NOEXCEPT
    {
        HMPC_DEVICE_ASSERT(std::bit_width(value) <= 2);
        return (value >> 1) | ((value & 0x1u) << 1);
    }

    constexpr std::uint32_t bit_reverse(std::uint32_t value, hmpc::size_constant<4>) HMPC_NOEXCEPT
    {
        HMPC_DEVICE_ASSERT(std::bit_width(value) <= 4);
        value = ((value >> 1) & 0x5u) | ((value & 0x5u) << 1);
        return (value >> 2) | ((value & 0x3u) << 2);
    }

    constexpr std::uint32_t bit_reverse(std::uint32_t value, hmpc::size_constant<8>) HMPC_NOEXCEPT
    {
        HMPC_DEVICE_ASSERT(std::bit_width(value) <= 8);
        value = ((value >> 1) & 0x55u) | ((value & 0x55u) << 1);
        value = ((value >> 2) & 0x33u) | ((value & 0x33u) << 2);
        return (value >> 4) | ((value & 0xfu) << 4);
    }

    constexpr std::uint32_t bit_reverse(std::uint32_t value, hmpc::size_constant<16>) HMPC_NOEXCEPT
    {
        HMPC_DEVICE_ASSERT(std::bit_width(value) <= 16);
        value = ((value >> 1) & 0x5555u) | ((value & 0x5555u) << 1);
        value = ((value >> 2) & 0x3333u) | ((value & 0x3333u) << 2);
        value = ((value >> 4) & 0xf0fu) | ((value & 0xf0fu) << 4);
        return (value >> 8) | ((value & 0xffu) << 8);
    }

    constexpr std::uint32_t bit_reverse(std::uint32_t value, hmpc::size_constant<32>) HMPC_NOEXCEPT
    {
        value = ((value >> 1) & 0x5555'5555u) | ((value & 0x5555'5555u) << 1);
        value = ((value >> 2) & 0x3333'3333u) | ((value & 0x3333'3333u) << 2);
        value = ((value >> 4) & 0xf0f'0f0fu) | ((value & 0xf0f'0f0fu) << 4);
        value = ((value >> 8) & 0xff'00ffu) | ((value & 0xff'00ffu) << 8);
        return (value >> 16) | (value << 16);
    }

    template<hmpc::size N>
    constexpr std::uint32_t bit_reverse(std::uint32_t value, hmpc::size_constant<N>) HMPC_NOEXCEPT
    {
        static_assert(N >= 0);
        static_assert(N < 32);

        constexpr std::size_t n = N;

        constexpr auto next_power_of_two = std::bit_ceil(n);
        static_assert(n != next_power_of_two);

        return bit_reverse(value, hmpc::size_constant_of<next_power_of_two>) >> (next_power_of_two - n);
    }
}
