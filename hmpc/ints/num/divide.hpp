#pragma once

#include <hmpc/core/num/divide.hpp>

namespace hmpc::ints::num
{
    template<typename Quotient, typename Remainder, typename Numerator, typename Denominator>
    consteval void divide(Quotient& quotient, Remainder& remainder, Numerator const& numerator, Denominator const& denominator)
    {
        hmpc::core::num::divide(
            quotient.compiletime_span(hmpc::access::write),
            remainder.compiletime_span(hmpc::access::write),
            numerator.compiletime_span(hmpc::access::read),
            denominator.compiletime_span(hmpc::access::read)
        );
    }
}
