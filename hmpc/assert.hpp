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
            static_assert(value.value);
            return;
        }
        else
        {
            return (value) ? static_cast<void>(0) : throw hmpc::compiletime_error();
        }
    }

    namespace detail
    {
        template<typename Value>
        constexpr bool boolify(Value value) HMPC_NOEXCEPT
        {
            if constexpr (hmpc::is_constant<Value>)
            {
                return static_cast<bool>(value.value);
            }
            else
            {
                return static_cast<bool>(value);
            }
        }
    }
}
