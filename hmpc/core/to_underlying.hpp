#pragma once

#include <hmpc/constant.hpp>

#include <type_traits>
#include <utility>

namespace hmpc::core
{
    template<typename Value>
    constexpr auto to_underlying(Value value)
    {
        if constexpr (hmpc::is_constant<Value>)
        {
            return hmpc::constant_of<std::to_underlying(value.value)>;
        }
        else
        {
            return std::to_underlying(value);
        }
    }
}
