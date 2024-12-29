#pragma once

#include <hmpc/constant.hpp>

#include <bit>

namespace hmpc::core
{
    template<hmpc::maybe_constant_of<hmpc::bit> Value>
    constexpr auto has_single_bit(Value value) HMPC_NOEXCEPT
    {
        return value;
    }

    template<typename Value>
    constexpr auto has_single_bit(Value value) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Value>)
        {
            return hmpc::constant_of<hmpc::bit{std::has_single_bit(value.value.data)}>;
        }
        else
        {
            return hmpc::bit{std::has_single_bit(value.data)};
        }
    }
}
