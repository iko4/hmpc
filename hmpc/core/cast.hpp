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

    template<hmpc::is_constant From>
    constexpr auto bool_cast(From from) noexcept
    {
        return hmpc::bool_constant_of<hmpc::bool_cast(from.value)>;
    }
}
