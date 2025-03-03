#pragma once

#include <hmpc/core/num/shift_right.hpp>

namespace hmpc::ints::num
{
    template<hmpc::size Shift, typename Result, typename Value>
    constexpr void shift_right(Result& result, Value const& value, hmpc::size_constant<Shift> shift = {}) HMPC_NOEXCEPT
    {
        hmpc::core::num::shift_right(
            result.span(hmpc::access::write),
            value.span(hmpc::access::read),
            shift
        );
    }
}
