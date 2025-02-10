#pragma once

#include <hmpc/ints/integer_traits.hpp>
#include <hmpc/ints/mod.hpp>
#include <hmpc/random/number_generator.hpp>

namespace hmpc::ints::num::theory
{
    template<auto Modulus, hmpc::size Degree>
    constexpr hmpc::bit is_root_of_unity(hmpc::ints::mod<Modulus> const& value, hmpc::size_constant<Degree> degree = {}) HMPC_NOEXCEPT
    {
        static_assert(hmpc::detail::has_single_bit(Degree));

        using mod = hmpc::ints::mod<Modulus>;
        using traits = hmpc::ints::integer_traits<mod>;
        return pow(value, hmpc::constant_of<degree / 2>) != traits::one & pow(value, degree) == traits::one;
    }

    template<auto Modulus, hmpc::size Degree>
    consteval auto root_of_unity(hmpc::constant<decltype(Modulus), Modulus> = {}, hmpc::size_constant<Degree> degree = {})
    {
        // TODO: static_assert(is_prime(Modulus));

        using mod = hmpc::ints::mod<Modulus>;
        using traits = hmpc::ints::integer_traits<mod>;
        using limb_type = mod::limb_type;

        constexpr auto max = traits::max_unsigned;
        constexpr auto qr = divide(max, hmpc::ints::ubigint<hmpc::detail::bit_width(Degree), limb_type>{Degree});
        static_assert(qr.remainder == hmpc::ints::zero<limb_type>);
        constexpr auto exponent = qr.quotient;

        auto prng = hmpc::random::compiletime_number_generator();

        mod x;
        while (true)
        {
            // biased random number is okay here
            prng.uniform(x.span(hmpc::access::write));
            if (hmpc::ints::num::greater(x, max))
            {
                hmpc::ints::num::subtract(x, x, mod::modulus);
            }

            if (x != traits::zero)
            {
                auto g = pow(x, exponent);
                if (is_root_of_unity(g, degree))
                {
                    return g;
                }
            }
        }
    }
}
