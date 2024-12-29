#pragma once

#include <hmpc/core/bit_and.hpp>
#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/is_normal.hpp>
#include <hmpc/iter/scan_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::size Bit, hmpc::read_only_bit_span Value, typename Mask>
    constexpr auto is_normal(Value value, Mask mask, hmpc::size_constant<Bit> bit = {}) HMPC_NOEXCEPT
    {
        constexpr auto limb_bit_size = value.limb_bit_size;
        constexpr hmpc::size normal_limb = bit / limb_bit_size;
        constexpr hmpc::size normal_bit = (bit % limb_bit_size) + ((bit % limb_bit_size == 0) and bit != 0) * limb_bit_size;
        return hmpc::iter::scan_range<normal_limb, value.limb_size>([&](auto i, auto normal)
        {
            if constexpr (i == normal_limb)
            {
                return hmpc::core::bit_and(normal, hmpc::core::is_normal<normal_bit>(value.read(i), mask));
            }
            else
            {
                return hmpc::core::bit_and(normal, hmpc::core::is_normal<0>(value.read(i), mask));
            }
        }, hmpc::core::equal_to(mask, value.sign_mask()));
    }
}
