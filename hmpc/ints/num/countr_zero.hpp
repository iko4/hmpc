#pragma once

#include <hmpc/core/num/countr_zero.hpp>

namespace hmpc::ints::num
{
    template<typename Value>
    constexpr auto countr_zero(Value const& value) HMPC_NOEXCEPT
    {
        return hmpc::core::num::countr_zero(
            value.span(hmpc::access::read)
        );
    }
}
