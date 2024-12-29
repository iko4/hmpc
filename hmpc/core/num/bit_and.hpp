#pragma once

#include <hmpc/core/bit_and.hpp>
#include <hmpc/core/limb_span.hpp>
#include <hmpc/iter/for_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span Left, hmpc::read_only_limb_span Right>
        requires (hmpc::same_limb_types<Result, Left, Right>)
    constexpr void bit_and(Result result, Left left, Right right) HMPC_NOEXCEPT
    {
        hmpc::iter::for_range<result.limb_size>([&](auto i)
        {
            result.write(i, hmpc::core::bit_and(left.extended_read(i), right.extended_read(i)));
        });
    }
}
