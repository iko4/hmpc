#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/combined_shift_left.hpp>
#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/iter/for_each_reverse_range.hpp>
#include <hmpc/iter/prev.hpp>

namespace hmpc::core::num
{
    template<hmpc::size Shift, hmpc::write_only_bit_span Result, hmpc::read_only_bit_span Value>
        requires (hmpc::same_limb_types<Result, Value>)
    constexpr void shift_left(Result result, Value value, hmpc::size_constant<Shift> = {}) HMPC_NOEXCEPT
    {
        using limb_type = Result::limb_type;
        static_assert(Shift >= 0);
        hmpc::iter::for_each_reverse(hmpc::range(result.limb_size), [&](auto i)
        {
            constexpr auto limb_shift = hmpc::size_constant_of<Shift / value.limb_bit_size>;
            constexpr auto bit_shift = hmpc::size_constant_of<Shift % value.limb_bit_size>;

            if constexpr (i < limb_shift)
            {
                result.write(i, hmpc::zero_constant_of<limb_type>, hmpc::access::unnormal);
            }
            else
            {
                constexpr auto current_index = hmpc::iter::prev(i, limb_shift);
                auto current = value.extended_read(current_index);
                if constexpr (bit_shift == 0)
                {
                    result.write(i, current);
                }
                else
                {
                    auto previous = [&]()
                    {
                        if constexpr (i - limb_shift > 0)
                        {
                            return value.extended_read(hmpc::iter::prev(current_index));
                        }
                        else
                        {
                            return hmpc::zero_constant_of<limb_type>;
                        }
                    }();
                    result.write(i, hmpc::core::combined_shift_left(previous, current, bit_shift));
                }
            }
        });
    }

    template<hmpc::write_only_compiletime_bit_span Result, hmpc::read_only_compiletime_bit_span Value>
        requires (hmpc::same_limb_types<Result, Value>)
    consteval void shift_left(Result result, Value value, hmpc::size shift)
    {
        using limb_type = Result::limb_type;
        HMPC_COMPILETIME_ASSERT(shift >= 0);
        for (hmpc::size i = result.limb_size; i-- > 0;)
        {
            auto limb_shift = shift / value.limb_bit_size;
            auto bit_shift = shift % value.limb_bit_size;

            if (i < limb_shift)
            {
                result.write(i, hmpc::zero_constant_of<limb_type>, hmpc::access::unnormal);
            }
            else
            {
                auto current = value.extended_read(i - limb_shift);
                if (bit_shift == 0)
                {
                    result.write(i, current);
                }
                else
                {
                    auto previous = (i - limb_shift > 0) ? value.extended_read(i - limb_shift - 1) : limb_type{};
                    result.write(i, hmpc::core::combined_shift_left(previous, current, bit_shift));
                }
            }
        }
    }
}
