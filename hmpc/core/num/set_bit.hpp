#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/set_bit.hpp>

namespace hmpc::core::num
{
    template<hmpc::size Bit, hmpc::read_write_bit_span Result, hmpc::maybe_constant_of<hmpc::bit> Value>
    constexpr void set_bit(Result result, Value value, hmpc::size_constant<Bit> = {}) HMPC_NOEXCEPT
    {
        static_assert(Bit >= 0);
        static_assert(Bit < result.bit_size);
        constexpr auto limb = hmpc::size_constant_of<Bit / result.limb_bit_size>;
        constexpr auto bit = hmpc::size_constant_of<Bit % result.limb_bit_size>;
        result.write(limb, hmpc::core::set_bit(result.read(limb), value, bit), hmpc::access::unnormal);
    }
}
