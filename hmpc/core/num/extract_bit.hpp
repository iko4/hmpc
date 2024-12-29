#pragma once

#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/core/extract_bit.hpp>
#include <hmpc/core/limb_span.hpp>

namespace hmpc::core::num
{
    template<hmpc::size Bit, hmpc::read_only_limb_span Value>
    constexpr hmpc::bit extract_bit(Value value, hmpc::size_constant<Bit> = {}) HMPC_NOEXCEPT
    {
        static_assert(Bit >= 0);
        constexpr auto limb = hmpc::size_constant_of<Bit / value.limb_bit_size>;
        constexpr auto bit = hmpc::size_constant_of<Bit % value.limb_bit_size>;
        return hmpc::core::extract_bit(value.extended_read(limb), bit);
    }

    template<hmpc::read_only_compiletime_bit_span Value>
    consteval hmpc::bit extract_bit(Value value, hmpc::size bit) HMPC_NOEXCEPT
    {
        HMPC_COMPILETIME_ASSERT(bit >= 0);
        auto limb = bit / value.limb_bit_size;
        auto bit_in_limb = bit % value.limb_bit_size;
        return hmpc::core::extract_bit(value.extended_read(limb), bit_in_limb);
    }
}
