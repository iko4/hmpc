#pragma once

#include <hmpc/core/limb_traits.hpp>

namespace hmpc::core
{
    template<typename Left, typename Right>
        requires (hmpc::same_without_constant<Left, Right>)
    constexpr auto bit_or(Left left, Right right) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Left>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;

#define HMPC_OR return left bitor right;

        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            return hmpc::constant_of<left.value bitor right.value>;
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            if constexpr (left.value == limb_traits::all_zeros)
            {
                return right;
            }
            else if constexpr (left.value == limb_traits::all_ones)
            {
                return limb_traits::all_ones;
            }
            else
            {
                HMPC_OR
            }
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == limb_traits::all_zeros)
            {
                return left;
            }
            else if constexpr (right.value == limb_traits::all_ones)
            {
                return limb_traits::all_ones;
            }
            else
            {
                HMPC_OR
            }
        }
        else
        {
            HMPC_OR
        }
#undef HMPC_OR
    }
}
