#pragma once

#include <hmpc/core/num/extract_bit.hpp>

namespace hmpc::ints::num
{
    template<hmpc::size Bit, typename Value>
    constexpr auto extract_bit(Value const& value, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
    {
        return hmpc::core::num::extract_bit(
            value.span(hmpc::access::read),
            bit
        );
    }

    template<typename Value>
    consteval auto extract_bit(Value const& value, hmpc::size bit) HMPC_NOEXCEPT
    {
        return hmpc::core::num::extract_bit(
            value.compiletime_span(hmpc::access::read),
            bit
        );
    }
}
