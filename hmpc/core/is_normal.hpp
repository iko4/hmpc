#pragma once

#include <hmpc/core/equal_to.hpp>
#include <hmpc/core/limb_traits.hpp>
#include <hmpc/core/mask_inside.hpp>
#include <hmpc/core/mask_outside.hpp>

namespace hmpc::core
{
    template<hmpc::size Bit, typename Value>
    constexpr auto is_normal(Value value, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        static_assert(bit >= 0);
        static_assert(bit <= limb_traits::bit_size);
        if constexpr (bit == limb_traits::bit_size)
        {
            return hmpc::constants::bit::one;
        }
        else
        {
            return equal_to(mask_inside(value, bit), value);
        }
    }

    template<hmpc::size Bit, typename Value, typename Mask>
        requires (hmpc::same_without_constant<Value, Mask>)
    constexpr auto is_normal(Value value, Mask sign_mask, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        static_assert(bit >= 0);
        static_assert(bit <= limb_traits::bit_size);
        HMPC_DEVICE_ASSERT(sign_mask == limb_traits::all_zeros or sign_mask == limb_traits::all_ones);
        if constexpr (bit == limb_traits::bit_size)
        {
            return hmpc::constants::bit::one;
        }
        else
        {
            return equal_to(mask_outside(value, bit), mask_outside(sign_mask, bit));
        }
    }

    template<hmpc::size Bit, typename Value, hmpc::maybe_constant_of<hmpc::bit> Sign>
    constexpr auto is_normal(Value value, Sign sign, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
        requires (not hmpc::same_without_constant<Value, hmpc::bit>)
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        return is_normal(value, limb_traits::mask_from(sign), bit);
    }

    template<typename Value>
    consteval auto is_normal(Value value, hmpc::size bit)
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        HMPC_COMPILETIME_ASSERT(bit >= 0);
        HMPC_COMPILETIME_ASSERT(bit <= limb_traits::bit_size);
        if (bit == limb_traits::bit_size)
        {
            return hmpc::bit{true};
        }
        else
        {
            return equal_to(mask_inside(value, bit), value);
        }
    }
}
