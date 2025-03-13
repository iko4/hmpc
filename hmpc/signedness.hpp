#pragma once

#include <hmpc/core/bit_or.hpp>
#include <hmpc/core/cast.hpp>

namespace hmpc
{
    enum class signedness : bool
    {
        unsigned_ = false,
        signed_ = true
    };

    constexpr signedness without_sign = signedness::unsigned_;
    constexpr signedness with_sign = signedness::signed_;

    constexpr bool is_unsigned(signedness s) noexcept
    {
        return s == without_sign;
    }

    constexpr bool is_signed(signedness s) noexcept
    {
        return s == with_sign;
    }

    /// If one of the operands is signed, the result is signed as well.
    constexpr auto common_signedness(hmpc::maybe_constant_of<signedness> auto left, hmpc::maybe_constant_of<signedness> auto right) noexcept
    {
        return hmpc::core::cast<signedness>(hmpc::core::bit_or(hmpc::core::bool_cast(left), hmpc::core::bool_cast(right)));
    }
}
