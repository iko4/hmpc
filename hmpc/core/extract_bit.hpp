#pragma once

#include <hmpc/core/bit_and.hpp>
#include <hmpc/core/is_set.hpp>
#include <hmpc/core/limb_traits.hpp>
#include <hmpc/core/shift_left.hpp>

namespace hmpc::core
{

    template<hmpc::size Bit, typename Value>
    constexpr auto extract_bit(Value value, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        static_assert(bit >= 0);
        static_assert(bit < limb_traits::bit_size);
        if constexpr (bit == 0)
        {
            if constexpr (hmpc::is_constant<Value>)
            {
                return hmpc::constant_of<hmpc::bit{value.value}>;
            }
            else
            {
                return hmpc::bit{value};
            }
        }
        else
        {
            constexpr auto mask = shift_left(limb_traits::one, bit);
            return is_set(bit_and(value, mask));
        }
    }

    template<typename Value>
    consteval auto extract_bit(Value value, hmpc::size bit)
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        HMPC_COMPILETIME_ASSERT(bit >= 0);
        HMPC_COMPILETIME_ASSERT(bit < limb_traits::bit_size);
        if (bit == 0)
        {
            if constexpr (hmpc::is_constant<Value>)
            {
                return hmpc::bit{value.value};
            }
            else
            {
                return hmpc::bit{value};
            }
        }
        else
        {
            auto mask = shift_left(limb_traits::one, bit);
            return is_set(bit_and(value, mask));
        }
    }
}
