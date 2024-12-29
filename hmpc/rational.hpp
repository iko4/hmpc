#pragma once

#include <hmpc/config.hpp>

#include <numeric>

namespace hmpc
{
    template<typename Numerator, typename Denominator>
    struct rational
    {
        using numerator_type = Numerator;
        using denominator_type = Denominator;

        numerator_type num;
        denominator_type den;

        consteval rational(numerator_type num) HMPC_NOEXCEPT
            : num(num)
            , den(1)
        {
        }

        consteval rational(numerator_type num, denominator_type den) HMPC_NOEXCEPT
        {
            HMPC_DEVICE_ASSERT(den != 0);

            auto gcd = std::gcd(num, den);

            if constexpr (std::numeric_limits<denominator_type>::is_signed)
            {
                denominator_type denominator_sign = (den < 0) ? -1 : 1;
                this->num = denominator_sign * num / gcd;
                this->den = denominator_sign * den / gcd;
            }
            else
            {
                this->num = num / gcd;
                this->den = den / gcd;
            }
        }

        friend consteval rational operator*(rational left, rational right) HMPC_NOEXCEPT
        {
            return rational{left.num * right.num, left.den * right.den};
        }

        explicit consteval operator numerator_type() const HMPC_NOEXCEPT
        {
            return num / den;
        }
    };

    using rational_size = rational<hmpc::size, hmpc::size>;

    template<rational_size N>
    using rational_size_constant = hmpc::constant<rational_size, N>;

    template<hmpc::size N, hmpc::size D = 1>
    constexpr rational_size_constant<rational_size{N, D}> rational_size_constant_of = {};

    template<typename T>
    concept maybe_rational_size_constant = hmpc::is_constant_of<T, hmpc::size> or hmpc::is_constant_of<T, rational_size>;
}

namespace hmpc::constants
{
    constexpr auto half = hmpc::rational_size_constant_of<1, 2>;
}
