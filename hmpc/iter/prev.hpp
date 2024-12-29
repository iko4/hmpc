#pragma once

#include <hmpc/constant.hpp>

namespace hmpc::iter
{
    template<auto Index>
    constexpr auto prev(hmpc::constant<decltype(Index), Index> = {}) noexcept
    {
        using type = decltype(Index);
        static_assert(Index > 0);
        return hmpc::constant<type, Index - type{1}>{};
    }

    template<auto Offset, auto Index>
    constexpr auto prev(hmpc::constant<decltype(Index), Index> = {}, hmpc::constant<decltype(Offset), Offset> = {}) noexcept
    {
        using type = decltype(Index);
        static_assert(Index >= 0);
        static_assert(Offset >= 0);
        static_assert(Index >= Offset);
        return hmpc::constant<type, Index - Offset>{};
    }
}
