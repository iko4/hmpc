#pragma once

#include <hmpc/core/limb_traits.hpp>

#include <numeric>

namespace hmpc::core
{
    template<typename Left, typename Right>
        requires (hmpc::same_without_constant<Left, Right>)
    constexpr auto lcm(Left left, Right right) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            return hmpc::constant_of<std::lcm(left.value, right.value)>;
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            return std::lcm(left.value, right);
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            return std::lcm(left, right.value);
        }
        else
        {
            return std::lcm(left, right);
        }
    }
}
