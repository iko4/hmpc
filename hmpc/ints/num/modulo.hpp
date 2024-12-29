#pragma once

#include <hmpc/core/num/modulo.hpp>

namespace hmpc::ints::num
{
    template<typename Result, typename Value, hmpc::unsigned_read_only_bit_span Modulus, hmpc::size AuxiliaryPower, hmpc::is_constant_of<typename Result::limb_type> InverseModulus>
    constexpr void montgomery_reduce(Result& result, Value const& value, Modulus modulus, hmpc::size_constant<AuxiliaryPower> auxiliary_power, InverseModulus inverse_modulus) HMPC_NOEXCEPT
    {
        hmpc::core::num::montgomery_reduce(
            result.span(hmpc::access::write),
            value.span(hmpc::access::read),
            modulus,
            auxiliary_power,
            inverse_modulus
        );
    }
}
