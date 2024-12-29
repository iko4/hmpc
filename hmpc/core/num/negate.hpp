#pragma once

#include <hmpc/core/limb_span.hpp>
#include <hmpc/core/num/add.hpp>
#include <hmpc/core/num/bit_not.hpp>

namespace hmpc::core::num
{
    template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span Value>
        requires (hmpc::same_limb_types<Result, Value>)
    constexpr void negate(Result result, Value value) HMPC_NOEXCEPT
    {
        using limb_type = Result::limb_type;
        hmpc::core::bit_array<result.bit_size, limb_type, result.signedness> intermediate_result;

        bit_not(intermediate_result.span(hmpc::access::write), value);
        add(result, intermediate_result.span(hmpc::access::read), hmpc::core::read_nullspan<limb_type>, hmpc::constants::bit::one);
    }
}
