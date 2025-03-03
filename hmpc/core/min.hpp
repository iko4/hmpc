#pragma once

#include <hmpc/core/limb_traits.hpp>

#include <algorithm>

namespace hmpc::core
{
    template<typename Left, typename Right>
        requires (hmpc::same_without_constant<Left, Right>)
    constexpr auto min(Left left, Right right) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Left>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            return hmpc::constant_of<std::min(left, right)>;
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            if constexpr (left.value == limb_traits::min)
            {
                return hmpc::constant_of<limb_traits::min>;
            }
            else
            {
                return std::min(left, right);
            }
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == limb_traits::min)
            {
                return hmpc::constant_of<limb_traits::min>;
            }
            else
            {
                return std::min(left, right);
            }
        }
        else
        {
            return std::min(left, right);
        }
    }
}
