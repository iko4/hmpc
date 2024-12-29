#pragma once

#include <hmpc/core/num/greatest_common_divisor.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Left, typename Right>
    consteval void greatest_common_divisor(Result& result, Left const& left, Right const& right)
    {
        hmpc::core::num::greatest_common_divisor(
            result.compiletime_span(hmpc::access::write),
            left.compiletime_span(hmpc::access::read),
            right.compiletime_span(hmpc::access::read)
        );
    }

    template<typename Result, typename LeftResult, typename RightResult, typename Left, typename Right>
    consteval void extended_euclidean(Result& result, LeftResult& left_result, RightResult& right_result, Left const& left, Right const& right)
    {
        hmpc::core::num::extended_euclidean(
            result.compiletime_span(hmpc::access::write),
            left_result.compiletime_span(hmpc::access::write),
            right_result.compiletime_span(hmpc::access::write),
            left.compiletime_span(hmpc::access::read),
            right.compiletime_span(hmpc::access::read)
        );
    }
}
