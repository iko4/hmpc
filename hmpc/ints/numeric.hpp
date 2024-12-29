#pragma once

#include <hmpc/core/select_bit_span.hpp>
#include <hmpc/ints/bigint.hpp>
#include <hmpc/ints/num/bit_width.hpp>
#include <hmpc/ints/num/greatest_common_divisor.hpp>
#include <hmpc/ints/num/has_single_bit.hpp>

namespace hmpc::ints
{
    template<hmpc::size Bits, hmpc::signedness Signedness, typename Limb, typename Normalization>
    constexpr auto has_single_bit(bigint<Bits, Signedness, Limb, Normalization> const& value) HMPC_NOEXCEPT
    {
        return hmpc::ints::num::has_single_bit(value);
    }
    template<hmpc::size Bits, hmpc::signedness Signedness, typename Limb, typename Normalization>
    constexpr auto bit_width(bigint<Bits, Signedness, Limb, Normalization> const& value) HMPC_NOEXCEPT
    {
        return hmpc::ints::num::bit_width(value);
    }

    template<typename T>
    constexpr auto abs(T const& value) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_signed(T::signedness))
        {
            using limb_type = T::limb_type;
            using normal_type = T::normal_type;

            using result_type = hmpc::ints::ubigint<T::bit_size, limb_type, normal_type>;
            auto sign = value.span(hmpc::access::read).sign();

            auto negation = -value;
            return hmpc::ints::num::bit_copy<result_type>(
                hmpc::core::select_bit_span(
                    value.span(hmpc::access::read),
                    negation.span(hmpc::access::read),
                    sign
                )
            );
        }
        else
        {
            return value;
        }
    }

    template<typename Quotient, typename Remainder>
    struct divide_result
    {
        Quotient quotient;
        Remainder remainder;
    };

    template<typename Limb, typename Normalization, hmpc::size LeftBits, hmpc::size RightBits>
    consteval auto divide(ubigint<LeftBits, Limb, Normalization> const& left, ubigint<RightBits, Limb, Normalization> const& right)
    {
        ubigint<LeftBits, Limb, Normalization> quotient;
        ubigint<RightBits, Limb, Normalization> remainder;
        hmpc::ints::num::divide(quotient, remainder, left, right);
        return divide_result<decltype(quotient), decltype(remainder)>{quotient, remainder};
    }

    template<typename Limb, typename Normalization, hmpc::size LeftBits, hmpc::size RightBits>
    consteval auto greatest_common_divisor(ubigint<LeftBits, Limb, Normalization> const& left, ubigint<RightBits, Limb, Normalization> const& right)
    {
        ubigint<std::max(LeftBits, RightBits), Limb, Normalization> result;
        hmpc::ints::num::greatest_common_divisor(result, left, right);
        return result;
    }

    template<typename Limb, typename Normalization, hmpc::size Bits, hmpc::size ModulusBits>
    consteval auto invert_modulo(ubigint<Bits, Limb, Normalization> const& value, ubigint<ModulusBits, Limb, Normalization> const& modulus)
    {
        constexpr auto max_width = std::max(Bits, ModulusBits);
        ubigint<max_width, Limb, Normalization> greatest_common_divisor;
        ubigint<ModulusBits, Limb, Normalization> inverse;
        hmpc::ints::num::extended_euclidean(greatest_common_divisor, inverse, hmpc::core::compiletime_nullspan<Limb>, value, modulus);
        HMPC_COMPILETIME_ASSERT(greatest_common_divisor == ubigint<1>{1});
        return inverse;
    }
}
