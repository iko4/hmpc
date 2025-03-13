#pragma once

#include <hmpc/constant.hpp>

namespace hmpc::iter
{
    template<auto Index>
    constexpr auto next(hmpc::constant<decltype(Index), Index> = {}) noexcept
    {
        using type = decltype(Index);
        static_assert(Index >= 0);
        static_assert(Index < std::numeric_limits<type>::max());
        return hmpc::constant<type, Index + type{1}>{};
    }

    template<auto Offset, auto Index>
    constexpr auto next(hmpc::constant<decltype(Index), Index> = {}, hmpc::constant<decltype(Offset), Offset> = {}) noexcept
    {
        using type = decltype(Index);
        static_assert(Index >= 0);
        if constexpr (Offset < 0)
        {
            static_assert(Index + Offset >= 0);
        }
        else
        {
            static_assert(Offset >= 0);
            static_assert(std::numeric_limits<type>::max() - Offset >= Index);
        }
        return hmpc::constant<type, Index + Offset>{};
    }
}
