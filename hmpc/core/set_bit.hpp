#pragma once

#include <hmpc/core/bit_and.hpp>
#include <hmpc/core/bit_not.hpp>
#include <hmpc/core/bit_or.hpp>
#include <hmpc/core/limb_traits.hpp>
#include <hmpc/core/shift_left.hpp>

namespace hmpc::core
{
    template<hmpc::size Bit, typename Value, hmpc::maybe_constant_of<hmpc::bit> BitValue>
    constexpr auto set_bit(Value value, BitValue bit_value, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        static_assert(bit >= 0);
        static_assert(bit < limb_traits::bit_size);
        constexpr auto clear_mask = bit_not(shift_left(limb_traits::one, bit));
        auto mask = shift_left(limb_traits::value_from(bit_value), bit);
        return bit_or(bit_and(value, clear_mask), mask);
    }
}
