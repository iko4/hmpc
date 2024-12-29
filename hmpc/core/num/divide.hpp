#pragma once

#include <hmpc/core/divide.hpp>
#include <hmpc/core/num/add.hpp>
#include <hmpc/core/num/bit_copy.hpp>
#include <hmpc/core/num/bit_width.hpp>
#include <hmpc/core/num/compare.hpp>
#include <hmpc/core/num/extract_bit.hpp>
#include <hmpc/core/num/multiply.hpp>
#include <hmpc/core/num/set_bit.hpp>
#include <hmpc/core/num/shift_left.hpp>
#include <hmpc/core/num/shift_right.hpp>
#include <hmpc/core/num/subtract.hpp>

namespace hmpc::core::num
{
    /// #### Algorithm reference
    /// - [1, Algorithm 3.1] Karl Hasselström: "Fast Division of Large Integers." Royal Institute of Technology, 2003. [Link](https://citeseerx.ist.psu.edu/document?repid=rep1&type=pdf&doi=1dda5050f46ff5da648caee9e64e3125f4dde669), accessed 2023-08-31.
    /// - [2] Donald E. Knuth: "The Art of Computer Programming II: Seminumerical Algorithms." Second edition. Addison-Wesley, 1981.
    template<hmpc::write_only_compiletime_bit_span Quotient, hmpc::write_only_compiletime_bit_span Remainder, hmpc::read_only_compiletime_bit_span Numerator, hmpc::read_only_compiletime_bit_span Denominator>
        requires (hmpc::same_limb_types<Quotient, Remainder, Numerator, Denominator>)
    consteval void divide_similar_sized(Quotient quotient, Remainder remainder, Numerator numerator, Denominator denominator)
    {
        using limb_type = Quotient::limb_type;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        auto numerator_limb_size = hmpc::core::limb_size_for<limb_type>(bit_width(numerator));
        auto denominator_bit_size = bit_width(denominator);
        auto denominator_limb_size = hmpc::core::limb_size_for<limb_type>(denominator_bit_size);

        HMPC_COMPILETIME_ASSERT(denominator_bit_size % limb_traits::bit_size == 0);
        HMPC_COMPILETIME_ASSERT(numerator_limb_size <= denominator_limb_size + 1);

        hmpc::core::compiletime_bit_array<limb_type> intermediate(denominator_bit_size + limb_traits::bit_size);
        auto read_intermediate = intermediate.compiletime_span(hmpc::access::read);
        auto write_intermediate = intermediate.compiletime_span(hmpc::access::write);

        // intermediate = denominator * 2^limb_bit_size
        bit_copy(write_intermediate.subspan(1), denominator);
        // implicit: intermediate[0] = 0

        if (hmpc::core::num::greater_equal(numerator, read_intermediate))
        {
            subtract(write_intermediate, numerator, read_intermediate);

            hmpc::core::compiletime_bit_array<limb_type> intermediate_quotient(quotient.bit_size);
            auto read_intermediate_quotient = intermediate_quotient.compiletime_span(hmpc::access::read);
            auto write_intermediate_quotient = intermediate_quotient.compiletime_span(hmpc::access::write);
            divide_similar_sized(write_intermediate_quotient, remainder, read_intermediate, denominator);

            // quotient = intermediate_quotient + 2^limb_bit_size
            quotient.write(0, read_intermediate_quotient.read(0));
            add(quotient.subspan(1), read_intermediate_quotient.subspan(1), hmpc::core::read_compiletime_nullspan<limb_type>, hmpc::bit{true});
        }
        else
        {
            auto q = hmpc::core::clamped_divide(numerator.extended_read(denominator_limb_size - 1), numerator.extended_read(denominator_limb_size), denominator.read(denominator_limb_size - 1));
            auto read_intermediate_quotient = hmpc::core::compiletime_bit_span<limb_type, hmpc::access::read_tag, hmpc::access::normal_tag>{hmpc::core::bit_width(q), &q};

            multiply(write_intermediate, denominator, read_intermediate_quotient);

            if (hmpc::core::num::greater(read_intermediate, numerator))
            {
                q -= limb_traits::one;
                subtract(write_intermediate, read_intermediate, denominator);
            }

            if (hmpc::core::num::greater(read_intermediate, numerator))
            {
                q -= limb_traits::one;
                subtract(write_intermediate, read_intermediate, denominator);
            }

            bit_copy(quotient, read_intermediate_quotient);
            subtract(remainder, numerator, read_intermediate);
        }
    }

    /// #### Algorithm reference
    /// - [1, Algorithm 3.2] Karl Hasselström: "Fast Division of Large Integers." Royal Institute of Technology, 2003. [Link](https://citeseerx.ist.psu.edu/document?repid=rep1&type=pdf&doi=1dda5050f46ff5da648caee9e64e3125f4dde669), accessed 2023-08-31.
    /// - [2] Donald E. Knuth: "The Art of Computer Programming II: Seminumerical Algorithms." Second edition. Addison-Wesley, 1981.
    template<hmpc::write_only_compiletime_bit_span Quotient, hmpc::write_only_compiletime_bit_span Remainder, hmpc::read_only_compiletime_bit_span Numerator, hmpc::read_only_compiletime_bit_span Denominator>
        requires (hmpc::same_limb_types<Quotient, Remainder, Numerator, Denominator>)
    consteval void divide_normalized(Quotient quotient, Remainder remainder, Numerator numerator, Denominator denominator)
    {
        using limb_type = Quotient::limb_type;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        auto numerator_bit_size = bit_width(numerator);
        auto numerator_limb_size = hmpc::core::limb_size_for<limb_type>(numerator_bit_size);
        auto denominator_bit_size = bit_width(denominator);
        auto denominator_limb_size = hmpc::core::limb_size_for<limb_type>(denominator_bit_size);

        HMPC_COMPILETIME_ASSERT(denominator_bit_size % limb_traits::bit_size == 0);

        if (numerator_limb_size < denominator_limb_size)
        {
            bit_copy(quotient, hmpc::core::read_compiletime_nullspan<limb_type>);
            bit_copy(remainder, numerator);
        }
        else if (numerator_limb_size == denominator_limb_size)
        {
            if (hmpc::core::num::less(numerator, denominator))
            {
                bit_copy(quotient, hmpc::core::read_compiletime_nullspan<limb_type>);
                bit_copy(remainder, numerator);
            }
            else
            {
                bit_copy(quotient, hmpc::core::compiletime_bit_span<limb_type, hmpc::access::read_tag, hmpc::access::normal_tag>{1, &limb_traits::one.value});
                subtract(remainder, numerator, denominator);
            }
        }
        else if (numerator_limb_size == denominator_limb_size + 1)
        {
            divide_similar_sized(quotient, remainder, numerator, denominator);
        }
        else
        {
            HMPC_COMPILETIME_ASSERT(numerator_limb_size > denominator_limb_size + 1);
            auto limb_size = numerator_limb_size - denominator_limb_size - 1;

            hmpc::core::compiletime_bit_array<limb_type> intermediate(numerator_bit_size);
            auto read_intermediate = intermediate.compiletime_span(hmpc::access::read);
            auto write_intermediate = intermediate.compiletime_span(hmpc::access::write);

            // quoptient[upper]    = numerator[upper] / denominator
            // intermediate[upper] = numerator[upper] % denominator
            // intermediate[lower] = numerator[lower]
            divide_similar_sized(quotient.subspan(limb_size), write_intermediate.subspan(limb_size), numerator.subspan(limb_size), denominator);
            bit_copy(write_intermediate.first_limbs(limb_size), numerator.first_limbs(limb_size));

            // quotient[lower] = intermediate / denominator
            // remainder       = intermediate % denominator
            divide_normalized(quotient.first_limbs(limb_size), remainder, read_intermediate, denominator);
        }
    }

    template<hmpc::write_only_compiletime_bit_span Quotient, hmpc::write_only_compiletime_bit_span Remainder, hmpc::read_only_compiletime_bit_span Numerator, hmpc::read_only_compiletime_bit_span Denominator>
        requires (hmpc::same_limb_types<Quotient, Remainder, Numerator, Denominator>)
    consteval void divide(Quotient quotient, Remainder remainder, Numerator numerator, Denominator denominator)
    {
        using limb_type = Quotient::limb_type;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        auto numerator_bit_size = bit_width(numerator);
        auto denominator_bit_size = bit_width(denominator);

        HMPC_COMPILETIME_ASSERT(denominator_bit_size != 0);

        if (denominator_bit_size % limb_traits::bit_size == 0)
        {
            divide_normalized(quotient, remainder, numerator, denominator);
        }
        else
        {
            auto shift = limb_traits::bit_size - (denominator_bit_size % limb_traits::bit_size);

            hmpc::core::compiletime_bit_array<limb_type> shifted_numerator(numerator_bit_size + shift);
            hmpc::core::compiletime_bit_array<limb_type> shifted_denominator(denominator_bit_size + shift);
            hmpc::core::compiletime_bit_array<limb_type> shifted_remainder(remainder.bit_size + shift);

            shift_left(shifted_numerator.compiletime_span(hmpc::access::write), numerator, shift);
            shift_left(shifted_denominator.compiletime_span(hmpc::access::write), denominator, shift);

            divide_normalized(quotient, shifted_remainder.compiletime_span(hmpc::access::write), shifted_numerator.compiletime_span(hmpc::access::read), shifted_denominator.compiletime_span(hmpc::access::read));

            shift_right(remainder, shifted_remainder.compiletime_span(hmpc::access::read), shift);
        }
    }
}
