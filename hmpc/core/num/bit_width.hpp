#pragma once

#include <hmpc/core/add.hpp>
#include <hmpc/core/bit_and.hpp>
#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/bit_width.hpp>
#include <hmpc/core/bit_xor.hpp>
#include <hmpc/core/cast.hpp>
#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/core/equal_to.hpp>
#include <hmpc/core/multiply.hpp>
#include <hmpc/core/not_equal_to.hpp>
#include <hmpc/core/select.hpp>
#include <hmpc/iter/scan_reverse_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::read_only_bit_span Value>
    constexpr auto bit_width(Value value) HMPC_NOEXCEPT
    {
        auto width = hmpc::iter::scan_reverse(hmpc::range(value.limb_size), [&](auto i, auto previous)
        {
            auto width = hmpc::core::bit_width(
                hmpc::core::bit_xor( // xor sign_mask is a no-op for unsigned types
                    value.sign_mask(),
                    value.read(i, hmpc::access::normal)
                )
            );
            auto previously_undetermined = hmpc::core::equal_to(previous, hmpc::constants::zero);
            auto now_determined = hmpc::core::not_equal_to(width, hmpc::constants::zero);
            return hmpc::core::select(
                previous,
                hmpc::core::add(width, hmpc::core::multiply(i, value.limb_bit_size)),
                hmpc::core::bit_and(previously_undetermined, now_determined)
            );
        }, hmpc::constants::zero);
        if constexpr (hmpc::is_signed(value.signedness))
        {
            return hmpc::core::add(width, hmpc::core::cast<hmpc::size>(value.sign()));
        }
        else
        {
            return width;
        }
    }

    template<hmpc::read_only_compiletime_bit_span Value>
    consteval hmpc::size bit_width(Value value)
    {
        for (hmpc::size i = value.limb_size; i-- > 0;)
        {
            auto width = hmpc::core::bit_width(value.read(i));
            if (width != 0)
            {
                return width + i * value.limb_bit_size;
            }
        }
        return 0;
    }
}
