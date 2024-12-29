#pragma once

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
    constexpr signedness common_signedness(signedness left, signedness right) noexcept
    {
        return static_cast<signedness>(static_cast<bool>(left) | static_cast<bool>(right));
    }
}
