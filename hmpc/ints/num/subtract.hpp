#pragma once

#include <hmpc/core/num/subtract.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Left, typename Right, typename Borrow = hmpc::zero_constant<hmpc::bit>>
    constexpr auto subtract(Result& result, Left const& left, Right const& right, Borrow borrow = {}) HMPC_NOEXCEPT
    {
        return hmpc::core::num::subtract(
            result.span(hmpc::access::write),
            left.span(hmpc::access::read),
            right.span(hmpc::access::read),
            borrow
        );
    }
}
