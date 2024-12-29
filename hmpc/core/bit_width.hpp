#pragma once

#include <hmpc/constant.hpp>

#include <bit>

namespace hmpc::core
{
    template<typename Value>
    constexpr auto bit_width(Value value) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Value>)
        {
            return hmpc::size_constant_of<std::bit_width(value.value.data)>;
        }
        else
        {
            return hmpc::size{std::bit_width(value.data)};
        }
    }
}
