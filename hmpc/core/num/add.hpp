#pragma once

#include <hmpc/core/add.hpp>
#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/core/limb_span.hpp>
#include <hmpc/iter/scan_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span Left, hmpc::read_only_limb_span Right, typename Carry = hmpc::zero_constant<hmpc::bit>>
        requires (hmpc::same_limb_types<Result, Left, Right>)
    constexpr auto add(Result result, Left left, Right right, Carry carry = {}) HMPC_NOEXCEPT
    {
        return hmpc::iter::scan_range<result.limb_size>([&](auto i, auto carry)
        {
            auto [sum, new_carry] = hmpc::core::extended_add(left.extended_read(i), right.extended_read(i), carry);
            result.write(i, sum);
            return new_carry;
        }, carry);
    }

    template<hmpc::write_only_compiletime_bit_span Result, hmpc::read_only_compiletime_bit_span Left, hmpc::read_only_compiletime_bit_span Right>
        requires (hmpc::same_limb_types<Result, Left, Right>)
    consteval hmpc::bit add(Result result, Left left, Right right, hmpc::bit carry = {})
    {
        for (hmpc::size i = 0; i < result.limb_size; ++i)
        {
            auto [sum, new_carry] = hmpc::core::extended_add(left.extended_read(i), right.extended_read(i), carry);
            result.write(i, sum);
            carry = new_carry;
        }
        return carry;
    }
}
