#pragma once

#include <hmpc/core/bit_and.hpp>
#include <hmpc/core/limb_traits.hpp>

namespace hmpc::core
{
    template<hmpc::size Bit, typename Value>
    constexpr auto mask_outside(Value value, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        static_assert(bit >= 0);
        static_assert(bit <= limb_traits::bit_size);
        if constexpr (bit == 0)
        {
            return value;
        }
        else if constexpr (bit == limb_traits::bit_size)
        {
            return hmpc::zero_constant_of<limb_type>;
        }
        else
        {
            constexpr auto mask = hmpc::constant_of<(compl ((limb_traits::one << bit) - limb_traits::one))>;
            return bit_and(value, mask);
        }
    }

    template<typename Value>
    consteval hmpc::traits::remove_constant_t<Value> mask_outside(Value value, hmpc::size bit)
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        HMPC_COMPILETIME_ASSERT(bit >= 0);
        HMPC_COMPILETIME_ASSERT(bit <= limb_traits::bit_size);
        if (bit == 0)
        {
            return value;
        }
        else if (bit == limb_traits::bit_size)
        {
            return limb_type{};
        }
        else
        {
            auto mask = compl ((limb_traits::one << bit) - limb_traits::one);
            return bit_and(value, mask);
        }
    }
}
