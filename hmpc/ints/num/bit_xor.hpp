#pragma once

#include <hmpc/core/num/bit_xor.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Left, typename Right>
    constexpr void bit_xor(Result& result, Left const& left, Right const& right) HMPC_NOEXCEPT
    {
        hmpc::core::num::bit_xor(
            result.span(hmpc::access::write),
            left.span(hmpc::access::read),
            right.span(hmpc::access::read)
        );
    }
}
