#pragma once

#include <hmpc/core/bit_or.hpp>
#include <hmpc/core/limb_traits.hpp>
#include <hmpc/core/shift_left.hpp>
#include <hmpc/core/shift_right.hpp>
#include <hmpc/iter/prev.hpp>

namespace hmpc::core
{
    template<typename Lower, typename Upper, hmpc::size Shift>
        requires (hmpc::same_without_constant<Lower, Upper>)
    constexpr auto combined_shift_right(Lower lower, Upper upper, hmpc::size_constant<Shift> shift) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<Lower>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;

        auto complementary_shift = hmpc::iter::prev(limb_traits::bit_size, shift);

        return bit_or(shift_left(upper, complementary_shift), shift_right(lower, shift));
    }

    template<typename Lower, typename Upper>
        requires (hmpc::same_without_constant<Lower, Upper>)
    consteval auto combined_shift_right(Lower lower, Upper upper, hmpc::size shift)
    {
        using limb_type = hmpc::traits::remove_constant_t<Lower>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;

        auto complementary_shift = limb_traits::bit_size - shift;

        return bit_or(shift_left(upper, complementary_shift), shift_right(lower, shift));
    }
}
