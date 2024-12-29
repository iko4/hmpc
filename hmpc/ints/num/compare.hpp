#pragma once

#include <hmpc/core/num/compare.hpp>

namespace hmpc::ints::num
{
    template<typename Left, typename Right>
    constexpr auto equal_to(Left const& left, Right const& right) HMPC_NOEXCEPT
    {
        return hmpc::core::num::equal_to(
            left.span(hmpc::access::read),
            right.span(hmpc::access::read)
        );
    }

    template<typename Left, typename Right>
    constexpr auto not_equal_to(Left const& left, Right const& right) HMPC_NOEXCEPT
    {
        return hmpc::core::num::not_equal_to(
            left.span(hmpc::access::read),
            right.span(hmpc::access::read)
        );
    }

    template<typename Left, typename Right>
    constexpr auto less(Left const& left, Right const& right) HMPC_NOEXCEPT
    {
        return hmpc::core::num::less(
            left.span(hmpc::access::read),
            right.span(hmpc::access::read)
        );
    }

    template<typename Left, typename Right>
    constexpr auto less_equal(Left const& left, Right const& right) HMPC_NOEXCEPT
    {
        return hmpc::core::num::less_equal(
            left.span(hmpc::access::read),
            right.span(hmpc::access::read)
        );
    }

    template<typename Left, typename Right>
    constexpr auto greater(Left const& left, Right const& right) HMPC_NOEXCEPT
    {
        return hmpc::core::num::greater(
            left.span(hmpc::access::read),
            right.span(hmpc::access::read)
        );
    }

    template<typename Left, typename Right>
    constexpr auto greater_equal(Left const& left, Right const& right) HMPC_NOEXCEPT
    {
        return hmpc::core::num::greater_equal(
            left.span(hmpc::access::read),
            right.span(hmpc::access::read)
        );
    }
}
