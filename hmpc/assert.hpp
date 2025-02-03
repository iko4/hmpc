#pragma once

#include <hmpc/constant.hpp>

namespace hmpc
{
    struct compiletime_error
    {
    };
    template<typename Value>
    consteval void compiletime_assert(Value value)
    {
        if constexpr (hmpc::is_constant<Value>)
        {
            static_assert(hmpc::bool_cast(value.value));
            return;
        }
        else
        {
            return hmpc::bool_cast(value) ? static_cast<void>(0) : throw hmpc::compiletime_error();
        }
    }
}
