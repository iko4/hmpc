#pragma once

#include <hmpc/constant.hpp>

namespace hmpc::core
{
    template<typename Left, typename Right>
        requires (hmpc::same_without_constant<Left, Right>)
    constexpr auto equal_to(Left left, Right right) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            return hmpc::constant_of<hmpc::bit{left == right}>;
        }
        else
        {
            return hmpc::bit{left == right};
        }
    }
}
