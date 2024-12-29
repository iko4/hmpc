#pragma once

#include <hmpc/core/add.hpp>
#include <hmpc/core/uint.hpp>

#include <sycl/sycl.hpp>

namespace hmpc::core
{
    template<typename LowerLimb, typename UpperLimb>
    struct multiply_result
    {
        LowerLimb lower;
        UpperLimb upper;

        constexpr multiply_result(LowerLimb lower, UpperLimb upper) HMPC_NOEXCEPT
            : lower(lower)
            , upper(upper)
        {
        }
    };

    template<typename Left, typename Right>
        requires (hmpc::same_without_constant<Left, Right>)
    constexpr auto multiply(Left left, Right right) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Left>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;

        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            return hmpc::constant_of<left.value * right.value>;
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            if constexpr (left.value == limb_traits::zero)
            {
                return hmpc::zero_constant_of<limb_type>;
            }
            else if constexpr (left.value == limb_traits::one)
            {
                return right;
            }
            else
            {
                return left.value * right;
            }
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == limb_traits::zero)
            {
                return hmpc::zero_constant_of<limb_type>;
            }
            else if constexpr (right.value == limb_traits::one)
            {
                return left;
            }
            else
            {
                return left * right.value;
            }
        }
        else
        {
            return left * right;
        }
    }

    template<typename Left, typename Right>
        requires (hmpc::same_without_constant<Left, Right> and not hmpc::same_without_constant<Left, hmpc::bit>)
    constexpr auto extended_multiply(Left left, Right right) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Left>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        using extended_limb_type = hmpc::core::traits::extended_limb_type_t<limb_type>;
#define HMPC_MULTIPLY(LEFT, RIGHT) \
    if consteval \
    { \
        auto extended_left = extended_limb_type{LEFT}; \
        auto extended_right = extended_limb_type{RIGHT}; \
        auto extended_result = extended_left * extended_right; \
        return multiply_result{limb_type{extended_result}, limb_type{extended_result >> hmpc::size_constant_of<limb_traits::bit_size>}}; \
    } \
    else \
    { \
        return multiply_result{LEFT * RIGHT, limb_type{sycl::mul_hi(LEFT.data, RIGHT.data)}}; \
    }
        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            constexpr auto extended_left = extended_limb_type{left.value};
            constexpr auto extended_right = extended_limb_type{right.value};
            constexpr auto extended_result = extended_left * extended_right;
            return multiply_result{hmpc::constant_of<limb_type{extended_result}>, hmpc::constant_of<limb_type{extended_result >> hmpc::size_constant_of<limb_type::bit_size>}>};
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            if constexpr (left.value == limb_traits::zero)
            {
                return multiply_result{hmpc::zero_constant_of<limb_type>, hmpc::zero_constant_of<limb_type>};
            }
            else if constexpr (left.value == limb_traits::one)
            {
                return multiply_result{right, hmpc::zero_constant_of<limb_type>};
            }
            else
            {
                HMPC_MULTIPLY(left.value, right)
            }
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == limb_traits::zero)
            {
                return multiply_result{hmpc::zero_constant_of<limb_type>, hmpc::zero_constant_of<limb_type>};
            }
            else if constexpr (right.value == limb_traits::one)
            {
                return multiply_result{left, hmpc::zero_constant_of<limb_type>};
            }
            else
            {
                HMPC_MULTIPLY(left, right.value)
            }
        }
        else
        {
            HMPC_MULTIPLY(left, right)
        }
#undef HMPC_MULTIPLY
    }

    template<hmpc::maybe_constant_of<hmpc::bit> Left, hmpc::maybe_constant_of<hmpc::bit> Right>
    constexpr auto extended_multiply(Left left, Right right) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            return multiply_result{hmpc::constant_of<left.value * right.value>, hmpc::constants::bit::zero};
        }
        else
        {
            return multiply_result{left * right, hmpc::constants::bit::zero};
        }
    }

    template<typename Left, typename Right, typename Summand, typename OtherSummand>
        requires (hmpc::same_without_constant<Left, Right, Summand, OtherSummand>)
    constexpr auto extended_multiply_add(Left left, Right right, Summand summand, OtherSummand other_summand) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Left>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        auto [lower, upper] = extended_multiply(left, right);
        auto [sum0, carry0] = extended_add(lower, summand);
        auto [sum1, carry1] = extended_add(sum0, other_summand);
        return multiply_result{sum1, add(add(upper, limb_traits::value_from(carry0)), limb_traits::value_from(carry1))};
    }

    template<typename Left, typename Right, typename Summand, typename OtherSummand>
        requires (hmpc::same_without_constant<Left, Right, Summand, OtherSummand>)
    constexpr auto multiply_add(Left left, Right right, Summand summand, OtherSummand other_summand) HMPC_NOEXCEPT
    {
        return add(multiply(left, right), add(summand, other_summand));
    }
}
