#pragma once

#include <hmpc/constant.hpp>

namespace hmpc::core
{
    template<typename To, typename From>
    constexpr auto cast(From from) noexcept
    {
        if constexpr (hmpc::is_constant<From>)
        {
            return hmpc::constant_of<static_cast<To>(from.value)>;
        }
        else
        {
            return static_cast<To>(from);
        }
    }
}
