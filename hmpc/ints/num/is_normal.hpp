#pragma once

#include <hmpc/core/num/is_normal.hpp>

namespace hmpc::ints::num
{
    template<hmpc::size Bit, typename Value, typename Mask>
    constexpr auto is_normal(Value const& value, Mask mask, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
    {
        return hmpc::core::num::is_normal(
            value.span(hmpc::access::read),
            mask,
            bit
        );
    }
}
