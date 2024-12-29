#pragma once

#include <hmpc/core/num/select.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename FalseValue, typename TrueValue, hmpc::maybe_constant_of<hmpc::bit> Choice>
    constexpr void select(Result& result, FalseValue const& false_value, TrueValue const& true_value, Choice choice) HMPC_NOEXCEPT
    {
        hmpc::core::num::select(
            result.span(hmpc::access::write),
            false_value.span(hmpc::access::read),
            true_value.span(hmpc::access::read),
            choice
        );
    }
}
