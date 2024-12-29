#pragma once

#include <hmpc/core/limb_traits.hpp>

namespace hmpc::core
{
    template<typename Left, typename Right>
        requires (hmpc::same_without_constant<Left, Right>)
    constexpr auto bit_xor(Left left, Right right) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Left>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;

#define HMPC_XOR return left xor right;

        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            return hmpc::constant_of<left.value xor right.value>;
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            if constexpr (left.value == limb_traits::all_zeros)
            {
                return right;
            }
            else if constexpr (left.value == limb_traits::all_ones)
            {
                return compl right;
            }
            else
            {
                HMPC_XOR
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
                return compl left;
            }
            else
            {
                HMPC_XOR
            }
        }
        else
        {
            HMPC_XOR
        }
#undef HMPC_XOR
    }
}
