#pragma once

#include <hmpc/constant.hpp>

namespace hmpc::iter
{
    template<auto Ratio, auto Index>
    constexpr auto stride(hmpc::constant<decltype(Index), Index> = {}, hmpc::constant<decltype(Ratio), Ratio> = {}) noexcept
    {
        using type = decltype(Index);
        static_assert(Index >= 0);
        static_assert(Ratio >= 0);
        // TODO: check that result fits type
        return hmpc::constant<type, Index * Ratio>{};
    }
}
