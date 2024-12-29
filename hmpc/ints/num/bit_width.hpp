#pragma once

#include <hmpc/core/num/bit_width.hpp>

namespace hmpc::ints::num
{
    template<typename Value>
    constexpr auto bit_width(Value const& value) HMPC_NOEXCEPT
    {
        return hmpc::core::num::bit_width(
            value.span(hmpc::access::read)
        );
    }
}
