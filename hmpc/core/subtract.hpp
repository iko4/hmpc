#pragma once

#include <hmpc/core/limb_traits.hpp>
#include <hmpc/core/uint.hpp>

namespace hmpc::core
{
    template<typename Limb, typename Borrow>
    struct [[nodiscard]] sub_result
    {
        Limb difference;
        Borrow borrow;

        constexpr sub_result(Limb difference, Borrow borrow) HMPC_NOEXCEPT
            : difference(difference)
            , borrow(borrow)
        {
        }
    };

    template<typename Left, typename Right>
    constexpr auto subtract(Left left, Right right) HMPC_NOEXCEPT
    {
        using left_limb_type = hmpc::traits::remove_constant_t<Left>;
        using left_limb_traits = hmpc::core::limb_traits<left_limb_type>;
        using right_limb_type = hmpc::traits::remove_constant_t<Right>;
        using right_limb_traits = hmpc::core::limb_traits<right_limb_type>;

#define HMPC_SUBTRACT \
    left_limb_type tmp = left - left_limb_traits::value_from(right); \
    return sub_result{tmp, left < tmp};

        static_assert(std::same_as<left_limb_type, right_limb_type> or std::same_as<right_limb_type, hmpc::bit>);

        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            constexpr left_limb_type tmp_left = left - left_limb_traits::value_from(right);
            constexpr hmpc::bit tmp_borrow = (left < tmp_left);
            return sub_result{hmpc::constant_of<tmp_left>, hmpc::constant_of<tmp_borrow>};
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == right_limb_traits::zero)
            {
                return sub_result{left, hmpc::constants::bit::zero};
            }
            else
            {
                HMPC_SUBTRACT
            }
        }
        else
        {
            HMPC_SUBTRACT
        }
#undef HMPC_SUBTRACT
    }

    template<typename Left, typename Right, hmpc::maybe_constant_of<hmpc::bit> Borrow>
        requires (hmpc::same_without_constant<Left, Right>)
    constexpr auto subtract(Left left, Right right, Borrow borrow) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Left>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;

#define HMPC_SUBTRACT \
    limb_type tmp_right = right + limb_traits::value_from(borrow); \
    limb_type tmp_left = left - tmp_right; \
    return sub_result{tmp_left, (left < tmp_left) | (right > tmp_right)};

        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right> and hmpc::is_constant<Borrow>)
        {
            constexpr limb_type tmp_right = right + limb_type{borrow};
            constexpr limb_type tmp_left = left - tmp_right;
            return sub_result{hmpc::constant_of<tmp_left>, hmpc::constant_of<(left < tmp_left) | (right > tmp_right)>};
        }
        else if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            constexpr auto tmp = subtract(left, right);
            auto [result, second_borrow] = subtract(tmp.difference, borrow);
            return sub_result{result, hmpc::core::bit_or(tmp.borrow, second_borrow)};
        }
        else if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Borrow>)
        {
            if constexpr (borrow)
            {
                if constexpr (left.value > limb_traits::zero)
                {
                    return subtract(hmpc::constant_of<left.value - limb_traits::value_from(borrow)>, right);
                }
                else
                {
                    return sub_result{limb_traits::max - right, hmpc::constants::bit::one};
                }
            }
            else
            {
                return subtract(left, right);
            }
        }
        else if constexpr (hmpc::is_constant<Right> and hmpc::is_constant<Borrow>)
        {
            if constexpr (borrow.value)
            {
                if constexpr (right.value == limb_traits::max)
                {
                    return sub_result{left, hmpc::constants::bit::one};
                }
                else
                {
                    return subtract(left, hmpc::constant_of<right.value + limb_traits::value_from(borrow)>);
                }
            }
            else
            {
                return subtract(left, right);
            }
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            if constexpr (right.value == limb_traits::zero)
            {
                return subtract(left, borrow);
            }
            else
            {
                HMPC_SUBTRACT
            }
        }
        else if constexpr (hmpc::is_constant<Borrow>)
        {
            if constexpr (borrow.value == hmpc::constants::bit::zero)
            {
                return subtract(left, right);
            }
            else
            {
                HMPC_SUBTRACT
            }
        }
        else
        {
            HMPC_SUBTRACT
        }
#undef HMPC_SUBTRACT
    }
}
