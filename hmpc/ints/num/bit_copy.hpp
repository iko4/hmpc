#pragma once

#include <hmpc/core/num/bit_copy.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Value>
    constexpr void bit_copy(Result& result, Value const& value) HMPC_NOEXCEPT
    {
        hmpc::core::num::bit_copy(
            result.span(hmpc::access::write),
            value.span(hmpc::access::read)
        );
    }

    template<typename Result, typename Value>
    constexpr Result bit_copy(Value const& value) HMPC_NOEXCEPT
    {
        Result result;
        bit_copy(result, value);
        return result;
    }
}
