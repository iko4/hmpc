#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/combined_shift_right.hpp>
#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/iter/for_range.hpp>
#include <hmpc/iter/next.hpp>

namespace hmpc::core::num
{
    template<hmpc::size Shift, hmpc::write_only_bit_span Result, hmpc::read_only_bit_span Value>
        requires (hmpc::same_limb_types<Result, Value>)
    constexpr void shift_right(Result result, Value value, hmpc::size_constant<Shift> = {}) HMPC_NOEXCEPT
    {
        static_assert(Shift >= 0);
        hmpc::iter::for_range<result.limb_size>([&](auto i)
        {
            constexpr auto limb_shift = hmpc::size_constant_of<Shift / value.limb_bit_size>;
            constexpr auto bit_shift = hmpc::size_constant_of<Shift % value.limb_bit_size>;

            constexpr auto current_index = hmpc::iter::next(i, limb_shift);

            auto current = value.extended_read(current_index, hmpc::access::normal);
            if constexpr (bit_shift == 0)
            {
                result.write(i, current);
            }
            else
            {
                auto next = value.extended_read(hmpc::iter::next(current_index), hmpc::access::normal);
                result.write(i, hmpc::core::combined_shift_right(current, next, bit_shift));
            }
        });
    }

    template<hmpc::write_only_compiletime_bit_span Result, hmpc::read_only_compiletime_bit_span Value>
        requires (hmpc::same_limb_types<Result, Value>)
    consteval void shift_right(Result result, Value value, hmpc::size shift)
    {
        HMPC_COMPILETIME_ASSERT(shift >= 0);
        for (hmpc::size i = 0; i < result.limb_size; ++i)
        {
            auto limb_shift = shift / value.limb_bit_size;
            auto bit_shift = shift % value.limb_bit_size;

            auto current = value.extended_read(i + limb_shift, hmpc::access::normal);
            if (bit_shift == 0)
            {
                result.write(i, current);
            }
            else
            {
                auto next = value.extended_read(i + limb_shift + 1, hmpc::access::normal);
                result.write(i, hmpc::core::combined_shift_right(current, next, bit_shift));
            }
        }
    }
}
