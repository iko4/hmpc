#pragma once

#include <hmpc/core/constant_bit_span.hpp>
#include <hmpc/core/size_limb_span.hpp>
#include <hmpc/ints/bigint.hpp>
#include <hmpc/random/number_generator.hpp>
#include <hmpc/rational.hpp>

namespace hmpc::random
{
    template<hmpc::size Count, hmpc::random::generator RandomGenerator = hmpc::random::number_generator<>>
    struct binomial_limits
    {
        using limb_type = RandomGenerator::value_type;

        /// Number of bits sampled
        static constexpr hmpc::size bit_count = Count;

        /// Bit size of result
        static constexpr hmpc::size bit_size = hmpc::detail::bit_width(Count);

        using value_type = hmpc::ints::ubigint<bit_size, limb_type, hmpc::access::unnormal_tag>;

        static constexpr hmpc::constant<value_type, value_type{}> min = {};
        static constexpr hmpc::constant<value_type, hmpc::constant_of<hmpc::ints::num::bit_copy<value_type>(hmpc::core::size_limb_span<limb_type>(Count))>> max = {};
    };

    template<hmpc::size Count, hmpc::random::generator RandomGenerator>
    constexpr auto binomial(RandomGenerator& random, hmpc::size_constant<Count> count = {}) HMPC_NOEXCEPT
    {
        static_assert(count > 0);

        using limits = binomial_limits<Count, RandomGenerator>;

        using limb_type = limits::limb_type;

        using value_type = limits::value_type;

        hmpc::core::bit_array<limits::bit_count, limb_type, hmpc::without_sign> bits;
        random.uniform(bits.span(hmpc::access::write));
        auto read_bits = bits.span(hmpc::access::read);

        value_type result;
        auto read_result = result.span(hmpc::access::read);
        auto write_result = result.span(hmpc::access::write);

        hmpc::iter::scan(hmpc::range(count), [&](auto i, auto current_limb)
        {
            if constexpr (i == 0)
            {
                hmpc::core::num::add(write_result, hmpc::core::read_nullspan<limb_type>, hmpc::core::read_nullspan<limb_type>, hmpc::bit{current_limb});
            }
            else
            {
                hmpc::core::num::add(write_result, read_result, hmpc::core::read_nullspan<limb_type>, hmpc::bit{current_limb});
            }

            constexpr hmpc::size bit = i % limb_type::bit_size;
            constexpr hmpc::size limb = i / limb_type::bit_size;

            if constexpr (bit == limb_type::bit_size - 1)
            {
                if constexpr (limb < read_bits.limb_size - 1)
                {
                    return read_bits.read(hmpc::size_constant_of<limb + 1>);
                }
                else
                {
                    return;
                }
            }
            else
            {
                if constexpr (i < count - 1)
                {
                    return current_limb >> hmpc::constants::one;
                }
                else
                {
                    return;
                }
            }
        }, read_bits.read(hmpc::constants::zero));

        return result;
    }

    template<hmpc::rational_size Variance, hmpc::random::generator RandomGenerator = hmpc::random::number_generator<>>
    struct centered_binomial_limits
    {
        static_assert((2 * Variance).den == 1);

        using limb_type = RandomGenerator::value_type;

        /// Number of bits sampled
        static constexpr hmpc::size bit_count = static_cast<hmpc::size>(4 * Variance);

        static constexpr hmpc::size mean = static_cast<hmpc::size>(2 * Variance);

        /// Bit size of result
        static constexpr hmpc::size bit_size = hmpc::detail::bit_width(mean) + 1;

        using value_type = hmpc::ints::sbigint<bit_size, limb_type, hmpc::access::unnormal_tag>;

        static constexpr hmpc::constant<value_type, hmpc::ints::num::bit_copy<value_type>(hmpc::core::size_limb_span<limb_type>(mean))> max = {};
        static constexpr hmpc::constant<value_type, -max.value> min = {};
    };

    template<hmpc::rational_size Variance, hmpc::random::generator RandomGenerator>
    constexpr auto centered_binomial(RandomGenerator& random, hmpc::rational_size_constant<Variance> = {}) HMPC_NOEXCEPT
    {
        using limits = centered_binomial_limits<Variance, RandomGenerator>;
        using limb_type = limits::limb_type;
        using value_type = limits::value_type;

        auto noncentered_binomial = binomial(random, hmpc::size_constant_of<limits::bit_count>);

        value_type result;
        hmpc::ints::num::subtract(result, noncentered_binomial, hmpc::core::constant_bit_span_from<hmpc::core::size_limb_span<limb_type>(limits::mean)>);

        return result;
    }

    template<hmpc::size Variance, hmpc::random::generator RandomGenerator>
    constexpr auto centered_binomial(RandomGenerator& random, hmpc::size_constant<Variance> = {}) HMPC_NOEXCEPT
    {
        return centered_binomial(random, hmpc::rational_size_constant_of<Variance>);
    }
}
