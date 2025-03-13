#pragma once

#include <hmpc/core/add.hpp>
#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/cast.hpp>
#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/core/countr_zero.hpp>
#include <hmpc/core/equal_to.hpp>
#include <hmpc/core/min.hpp>
#include <hmpc/core/multiply.hpp>
#include <hmpc/core/select.hpp>
#include <hmpc/iter/scan_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::unsigned_read_only_bit_span Value>
    constexpr hmpc::size countr_zero(Value value) HMPC_NOEXCEPT
    {
        return hmpc::core::min(hmpc::iter::scan(hmpc::range(value.limb_size), [&](auto i, auto previous)
        {
            auto count = hmpc::core::countr_zero(value.read(i));
            auto previously_undetermined = hmpc::core::cast<hmpc::bit>(
                hmpc::core::equal_to(
                    previous,
                    hmpc::size_constant_of<i * value.limb_bit_size>
                )
            );
            return hmpc::core::select(
                previous,
                hmpc::core::add(previous, count),
                previously_undetermined
            );
        }, hmpc::constants::zero), hmpc::size_constant_of<value.bit_size>);
    }

    template<hmpc::read_only_compiletime_bit_span Value>
    constexpr hmpc::size countr_zero(Value value) HMPC_NOEXCEPT
    {
        for (hmpc::size i = 0; i < value.limb_size; ++i)
        {
            auto count = hmpc::core::countr_zero(value.read(i));
            if (count != value.limb_bit_size)
            {
                return std::min(i * value.limb_bit_size + count, value.bit_size);
            }
        }
        return value.bit_size;
    }
}
