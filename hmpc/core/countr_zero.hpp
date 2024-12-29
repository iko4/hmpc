#pragma once

#include <hmpc/constant.hpp>

#include <bit>

namespace hmpc::core
{
    template<typename Value>
    constexpr auto countr_zero(Value value) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Value>)
        {
            return hmpc::size_constant_of<std::countr_zero(value.value.data)>;
        }
        else
        {
            return static_cast<hmpc::size>(std::countr_zero(value.data));
        }
    }
}
