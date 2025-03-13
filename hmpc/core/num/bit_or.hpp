#pragma once

#include <hmpc/core/bit_or.hpp>
#include <hmpc/core/limb_span.hpp>
#include <hmpc/iter/for_each_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span Left, hmpc::read_only_limb_span Right>
        requires (hmpc::same_limb_types<Result, Left, Right>)
    constexpr void bit_or(Result result, Left left, Right right) HMPC_NOEXCEPT
    {
        hmpc::iter::for_each(hmpc::range(result.limb_size), [&](auto i)
        {
            result.write(i, hmpc::core::bit_or(left.extended_read(i), right.extended_read(i)));
        });
    }
}
