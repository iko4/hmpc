#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/iter/for_each_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span Value>
        requires (hmpc::same_limb_types<Result, Value>)
    constexpr void bit_copy(Result result, Value value) HMPC_NOEXCEPT
    {
        hmpc::iter::for_each(hmpc::range(result.limb_size), [&](auto i)
        {
            result.write(i, value.extended_read(i));
        });
    }

    template<hmpc::write_only_compiletime_bit_span Result, hmpc::read_only_compiletime_bit_span Value>
        requires (hmpc::same_limb_types<Result, Value>)
    constexpr void bit_copy(Result result, Value value) HMPC_NOEXCEPT
    {
        for (hmpc::size i = 0; i < result.limb_size; ++i)
        {
            result.write(i, value.extended_read(i));
        }
    }
}
