#pragma once

#include <hmpc/core/bit_or.hpp>
#include <hmpc/core/limb_traits.hpp>
#include <hmpc/core/mask_outside.hpp>

namespace hmpc::core
{
    template<hmpc::size Bit, typename Value, typename Mask>
        requires (hmpc::same_without_constant<Value, Mask>)
    constexpr auto extend_sign(Value value, Mask sign_mask, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        static_assert(bit >= 0);
        static_assert(bit <= limb_traits::bit_size);
        HMPC_DEVICE_ASSERT(sign_mask == limb_traits::all_zeros or sign_mask == limb_traits::all_ones);

        if constexpr (bit == 0)
        {
            return sign_mask;
        }
        else if constexpr (bit == limb_traits::bit_size)
        {
            return value;
        }
        else
        {
            return bit_or(mask_outside(sign_mask, bit), value);
        }
    }

    template<hmpc::size Bit, typename Value, hmpc::maybe_constant_of<hmpc::bit> Mask>
    constexpr auto extend_sign(Value value, Mask sign, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
        requires (not hmpc::same_without_constant<Value, hmpc::bit>)
    {
        using limb_type = hmpc::traits::remove_constant_t<Value>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        return extend_sign(value, limb_traits::mask_from(sign), bit);
    }
}
