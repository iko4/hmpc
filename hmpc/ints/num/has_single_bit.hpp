#pragma once

#include <hmpc/core/num/has_single_bit.hpp>

namespace hmpc::ints::num
{
    template<typename Value>
    constexpr auto has_single_bit(Value const& value) HMPC_NOEXCEPT
    {
        return hmpc::core::num::has_single_bit(
            value.span(hmpc::access::read)
        );
    }
}
