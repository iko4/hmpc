#pragma once

#include <hmpc/core/num/bit_and.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Left, typename Right>
    constexpr void bit_and(Result& result, Left const& left, Right const& right) HMPC_NOEXCEPT
    {
        hmpc::core::num::bit_and(
            result.span(hmpc::access::write),
            left.span(hmpc::access::read),
            right.span(hmpc::access::read)
        );
    }
}