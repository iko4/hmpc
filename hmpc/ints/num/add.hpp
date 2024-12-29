#pragma once

#include <hmpc/core/num/add.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Left, typename Right, typename Carry = hmpc::zero_constant<hmpc::bit>>
    constexpr auto add(Result& result, Left const& left, Right const& right, Carry carry = {}) HMPC_NOEXCEPT
    {
        return hmpc::core::num::add(
            result.span(hmpc::access::write),
            left.span(hmpc::access::read),
            right.span(hmpc::access::read),
            carry
        );
    }
}
