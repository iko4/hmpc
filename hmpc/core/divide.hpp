#pragma once

#include <hmpc/core/limb_traits.hpp>
#include <hmpc/detail/utility.hpp>

namespace hmpc::core
{
    template<typename LowerNumerator, typename UpperNumerator, typename Denominator>
        requires (hmpc::same_without_constant<LowerNumerator, UpperNumerator, Denominator>)
    consteval auto clamped_divide(LowerNumerator lower_numerator, UpperNumerator upper_numerator, Denominator denominator)
    {
        using limb_type = hmpc::traits::remove_constant_t<LowerNumerator>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        using extended_limb_type = hmpc::core::traits::extended_limb_type_t<limb_type>;

        auto numerator = extended_limb_type{lower_numerator} | (extended_limb_type{upper_numerator} << limb_traits::bit_size);
        auto quotient = numerator / extended_limb_type{denominator};
        if (quotient > extended_limb_type{limb_traits::max.value})
        {
            return limb_traits::max.value;
        }
        else
        {
            return limb_type{quotient};
        }
    }

    template<typename Numerator, typename Denominator>
        requires (hmpc::same_without_constant<Numerator, Denominator>)
    constexpr auto ceil_divide(Numerator numerator, Denominator denominator)
    {
        if constexpr (hmpc::is_constant<Numerator> and hmpc::is_constant<Denominator>)
        {
            return hmpc::constant_of<hmpc::detail::div_ceil(numerator.value, denominator.value)>;
        }
        else if constexpr (hmpc::is_constant<Numerator>)
        {
            return hmpc::detail::div_ceil(numerator.value, denominator);
        }
        else if constexpr (hmpc::is_constant<Denominator>)
        {
            return hmpc::detail::div_ceil(numerator, denominator.value);
        }
        else
        {
            return hmpc::detail::div_ceil(numerator, denominator);
        }
    }
}
