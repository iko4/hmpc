#pragma once

#include <hmpc/constants.hpp>
#include <hmpc/core/bit_or.hpp>
#include <hmpc/core/limb_traits.hpp>
#include <hmpc/core/uint.hpp>

namespace hmpc::core
{
    template<typename Limb, typename Carry>
    struct [[nodiscard]] add_result
    {
        Limb sum;
        Carry carry;

        constexpr add_result(Limb sum, Carry carry) HMPC_NOEXCEPT
            : sum(sum)
            , carry(carry)
        {
        }
    };

    template<typename Left, typename Right>
    constexpr auto add(Left left, Right right) HMPC_NOEXCEPT
    {
        using left_limb_type = hmpc::traits::remove_constant_t<Left>;
        using left_limb_traits = hmpc::core::limb_traits<left_limb_type>;
        using right_limb_type = hmpc::traits::remove_constant_t<Right>;
        using right_limb_traits = hmpc::core::limb_traits<right_limb_type>;

        static_assert(std::same_as<left_limb_type, right_limb_type> or std::same_as<right_limb_type, hmpc::bit>);

#define HMPC_ADD \
    return left + left_limb_traits::value_from(right);

        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            return hmpc::constant_of<left + left_limb_traits::value_from(right)>;
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            if constexpr (left.value == left_limb_traits::zero)
            {
                return right;
            }
            else
            {
                HMPC_ADD
            }
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == right_limb_traits::zero)
            {
                return left;
            }
            else
            {
                HMPC_ADD
            }
        }
        else
        {
            HMPC_ADD
        }
#undef HMPC_ADD
    }

    template<typename Left, typename Right>
    constexpr auto extended_add(Left left, Right right) HMPC_NOEXCEPT
    {
        using left_limb_type = hmpc::traits::remove_constant_t<Left>;
        using left_limb_traits = hmpc::core::limb_traits<left_limb_type>;
        using right_limb_type = hmpc::traits::remove_constant_t<Right>;
        using right_limb_traits = hmpc::core::limb_traits<right_limb_type>;

        static_assert(std::same_as<left_limb_type, right_limb_type> or std::same_as<right_limb_type, hmpc::bit>);

#define HMPC_ADD \
    left_limb_type tmp = left + left_limb_traits::value_from(right); \
    return add_result{tmp, left > tmp};

        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            constexpr left_limb_type tmp = left + left_limb_traits::value_from(right);
            constexpr hmpc::bit carry = (left > tmp);
            return add_result{hmpc::constant_of<tmp>, hmpc::constant_of<carry>};
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            if constexpr (left.value == left_limb_traits::zero)
            {
                return add_result{left_limb_traits::value_from(right), hmpc::constants::bit::zero};
            }
            else
            {
                HMPC_ADD
            }
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == right_limb_traits::zero)
            {
                return add_result{left, hmpc::constants::bit::zero};
            }
            else
            {
                HMPC_ADD
            }
        }
        else
        {
            HMPC_ADD
        }
#undef HMPC_ADD
    }

    template<typename Left, typename Right, hmpc::maybe_constant_of<hmpc::bit> Carry>
        requires (hmpc::same_without_constant<Left, Right>)
    constexpr auto extended_add(Left left, Right right, Carry carry) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Left>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;

#define HMPC_ADD \
    limb_type tmp_left = left + limb_traits::value_from(carry); \
    limb_type tmp_right = right + tmp_left; \
    return add_result{tmp_right, (left > tmp_left) | (right > tmp_right)};

        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right> and hmpc::is_constant<Carry>)
        {
            constexpr limb_type tmp_left = left + limb_traits::value_from(carry);
            constexpr limb_type tmp_right = right + tmp_left;
            return add_result{hmpc::constant_of<tmp_right>, hmpc::constant_of<(left > tmp_left) | (right > tmp_right)>};
        }
        else if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            constexpr auto tmp = extended_add(left, right);
            auto [result, second_carry] = extended_add(tmp.sum, carry);
            return add_result{result, hmpc::core::bit_or(tmp.carry, second_carry)};
        }
        else if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Carry>)
        {
            constexpr auto tmp = extended_add(left, carry);
            auto [result, second_carry] = extended_add(tmp.sum, right);
            return add_result{result, hmpc::core::bit_or(tmp.carry, second_carry)};
        }
        else if constexpr (hmpc::is_constant<Right> and hmpc::is_constant<Carry>)
        {
            constexpr auto tmp = extended_add(right, carry);
            auto [result, second_carry] = extended_add(tmp.sum, left);
            return add_result{result, hmpc::core::bit_or(tmp.carry, second_carry)};
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            if constexpr (left.value == limb_traits::zero)
            {
                return extended_add(right, carry);
            }
            else
            {
                HMPC_ADD
            }
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == limb_traits::zero)
            {
                return extended_add(left, carry);
            }
            else
            {
                HMPC_ADD
            }
        }
        else if constexpr (hmpc::is_constant<Carry>)
        {
            if constexpr (carry.value == hmpc::constants::bit::zero)
            {
                return extended_add(right, left);
            }
            else
            {
                HMPC_ADD
            }
        }
        else
        {
            HMPC_ADD
        }
#undef HMPC_ADD
    }
}
