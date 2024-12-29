#pragma once

#include <hmpc/core/add.hpp>
#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/cast.hpp>
#include <hmpc/core/equal_to.hpp>
#include <hmpc/core/has_single_bit.hpp>
#include <hmpc/iter/scan_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::unsigned_read_only_bit_span Value>
    constexpr auto has_single_bit(Value value) HMPC_NOEXCEPT
    {
        // TODO: rewrite this by not just counting the single_bit-limbs and comparing the result to one
        return hmpc::core::equal_to(
            hmpc::iter::scan_range<value.limb_size>([&](auto i, auto previous)
            {
                auto single_bit = hmpc::core::has_single_bit(value.read(i, hmpc::access::normal));
                return hmpc::core::add(previous, hmpc::core::cast<hmpc::size>(single_bit));
            }, hmpc::constants::zero),
            hmpc::constants::one
        );
    }
}
