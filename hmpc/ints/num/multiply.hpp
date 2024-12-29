#pragma once

#include <hmpc/core/num/multiply.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Left, typename Right>
    constexpr void multiply(Result& result, Left const& left, Right const& right) HMPC_NOEXCEPT
    {
        hmpc::core::num::multiply(
            result.span(hmpc::access::write),
            left.span(hmpc::access::read),
            right.span(hmpc::access::read)
        );
    }
}
