#pragma once

#include <hmpc/ints/bigint.hpp>
#include <hmpc/ints/mod.hpp>
#include <hmpc/random/number_generator.hpp>

namespace hmpc::random
{
    template<hmpc::size Bits, hmpc::signedness Signedness, hmpc::random::generator RandomGenerator>
    constexpr auto uniform(RandomGenerator& random, hmpc::size_constant<Bits> bits = {}, hmpc::constant<hmpc::signedness, Signedness> = {}) noexcept
    {
        static_assert(bits >= 0);

        using limb_type = RandomGenerator::value_type;

        hmpc::ints::bigint<Bits, Signedness, limb_type> result;
        random.uniform(result.span(hmpc::access::write));

        return result;
    }

    template<hmpc::size Bits, hmpc::random::generator RandomGenerator>
    constexpr auto unsigned_uniform(RandomGenerator& random, hmpc::size_constant<Bits> bits = {}) noexcept
    {
        return uniform(random, bits, hmpc::constants::without_sign);
    }

    template<hmpc::size Bits, hmpc::random::generator RandomGenerator>
    constexpr auto signed_uniform(RandomGenerator& random, hmpc::size_constant<Bits> bits = {}) noexcept
    {
        return uniform(random, bits, hmpc::constants::with_sign);
    }

    template<auto Bound, hmpc::signedness Signedness, hmpc::random::generator RandomGenerator, hmpc::statistical_security StatisticalSecurity = hmpc::default_statistical_security>
    constexpr auto drown_uniform(RandomGenerator& random, hmpc::constant<decltype(Bound), Bound> = {}, hmpc::constant<hmpc::signedness, Signedness> signedness = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> security = {}) noexcept
    {
        using bound_type = decltype(Bound);
        using limb_type = bound_type::limb_type;
        static_assert(Bound > hmpc::ints::zero<limb_type>);
        constexpr auto max_value = (Bound - hmpc::ints::one<limb_type>) << hmpc::constant_cast<hmpc::size>(security);
        constexpr auto bits = bit_width(max_value) + hmpc::is_signed(signedness);

        hmpc::ints::bigint<bits, Signedness, limb_type> result;
        random.uniform(result.span(hmpc::access::write));

        return result;
    }

    template<auto Bound, hmpc::random::generator RandomGenerator, hmpc::statistical_security StatisticalSecurity = hmpc::default_statistical_security>
    constexpr auto drown_unsigned_uniform(RandomGenerator& random, hmpc::constant<decltype(Bound), Bound> bound = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> security = {}) noexcept
    {
        return drown_uniform(random, bound, hmpc::constants::without_sign, security);
    }

    template<auto Bound, hmpc::random::generator RandomGenerator, hmpc::statistical_security StatisticalSecurity = hmpc::default_statistical_security>
    constexpr auto drown_signed_uniform(RandomGenerator& random, hmpc::constant<decltype(Bound), Bound> bound = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> security = {}) noexcept
    {
        return drown_uniform(random, bound, hmpc::constants::with_sign, security);
    }

    template<typename IntegerModulo, hmpc::random::generator RandomGenerator, hmpc::statistical_security StatisticalSecurity = hmpc::default_statistical_security>
    constexpr auto uniform(RandomGenerator& random, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> security = {}) noexcept
    {
        using value_type = IntegerModulo;
        return value_type(drown_unsigned_uniform(random, value_type::modulus_constant, security), hmpc::ints::from_uniformly_random);
    }
}
