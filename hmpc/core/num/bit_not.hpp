#pragma once

#include <hmpc/core/bit_not.hpp>
#include <hmpc/core/limb_span.hpp>
#include <hmpc/iter/for_each_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span Value>
        requires (hmpc::same_limb_types<Result, Value>)
    constexpr void bit_not(Result result, Value value) HMPC_NOEXCEPT
    {
        hmpc::iter::for_each(hmpc::range(result.limb_size), [&](auto i)
        {
            result.write(i, hmpc::core::bit_not(value.extended_read(i)));
        });
    }
}
