#pragma once

#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/core/limb_span.hpp>
#include <hmpc/core/subtract.hpp>
#include <hmpc/iter/scan_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span Left, hmpc::read_only_limb_span Right, typename Borrow = hmpc::zero_constant<hmpc::bit>>
        requires (hmpc::same_limb_types<Result, Left, Right>)
    constexpr auto subtract(Result result, Left left, Right right, Borrow borrow = {}) HMPC_NOEXCEPT
    {
        return hmpc::iter::scan_range<result.limb_size>([&](auto i, auto borrow)
        {
            auto [difference, new_borrow] = hmpc::core::subtract(left.extended_read(i), right.extended_read(i), borrow);
            result.write(i, difference);
            return new_borrow;
        }, borrow);
    }

    template<hmpc::write_only_compiletime_bit_span Result, hmpc::read_only_compiletime_bit_span Left, hmpc::read_only_compiletime_bit_span Right>
        requires (hmpc::same_limb_types<Result, Left, Right>)
    constexpr hmpc::bit subtract(Result result, Left left, Right right, hmpc::bit borrow = {}) HMPC_NOEXCEPT
    {
        for (hmpc::size i = 0; i < result.limb_size; ++i)
        {
            auto [difference, new_borrow] = hmpc::core::subtract(left.extended_read(i), right.extended_read(i), borrow);
            result.write(i, difference);
            borrow = new_borrow;
        }
        return borrow;
    }
}
