#pragma once

#include <hmpc/core/num/bit_not.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Value>
    constexpr void bit_not(Result& result, Value const& value) HMPC_NOEXCEPT
    {
        hmpc::core::num::bit_not(
            result.span(hmpc::access::write),
            value.span(hmpc::access::read)
        );
    }
}
