#pragma once

#include <hmpc/core/num/negate.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Value>
    constexpr void negate(Result& result, Value const& value) HMPC_NOEXCEPT
    {
        hmpc::core::num::negate(
            result.span(hmpc::access::write),
            value.span(hmpc::access::read)
        );
    }
}
