#pragma once

#include <hmpc/constant.hpp>

namespace hmpc::core
{
    template<typename Value>
    constexpr auto is_set(Value value) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Value>)
        {
            return hmpc::constant_of<hmpc::bit{static_cast<bool>(value.value.data)}>;
        }
        else
        {
            return hmpc::bit{static_cast<bool>(value.data)};
        }
    }
}
