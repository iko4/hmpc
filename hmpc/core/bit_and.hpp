#pragma once

#include <hmpc/core/limb_traits.hpp>

namespace hmpc::core
{
    template<typename Left, typename Right>
        requires (hmpc::same_without_constant<Left, Right>)
    constexpr auto bit_and(Left left, Right right) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Left>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;

#define HMPC_AND return left bitand right;

        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            return hmpc::constant_of<left.value bitand right.value>;
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            if constexpr (left.value == limb_traits::all_zeros)
            {
                return hmpc::zero_constant_of<limb_type>;
            }
            else if constexpr (left.value == limb_traits::all_ones)
            {
                return right;
            }
            else
            {
                HMPC_AND
            }
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == limb_traits::all_zeros)
            {
                return hmpc::zero_constant_of<limb_type>;
            }
            else if constexpr (right.value == limb_traits::all_ones)
            {
                return left;
            }
            else
            {
                HMPC_AND
            }
        }
        else
        {
            HMPC_AND
        }
#undef HMPC_AND
    }
}
