#pragma once

#include <hmpc/core/bit_and.hpp>
#include <hmpc/core/bit_xor.hpp>
#include <hmpc/core/limb_traits.hpp>
#include <hmpc/core/uint.hpp>

#include <sycl/sycl.hpp>

namespace hmpc::core
{
    template<typename FalseValue, typename TrueValue, hmpc::maybe_constant_of<hmpc::bit> Choice>
        requires (hmpc::same_without_constant<FalseValue, TrueValue>)
    constexpr auto select(FalseValue false_value, TrueValue true_value, Choice choice) HMPC_NOEXCEPT
    {
        using limb_type = hmpc::traits::remove_constant_t<FalseValue>;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        if constexpr (hmpc::is_constant<Choice>)
        {
            if constexpr (choice.value)
            {
                return true_value;
            }
            else
            {
                return false_value;
            }
        }
        else if constexpr (not requires(limb_type x) { x.data; limb_traits::bit_size; } or std::same_as<limb_type, hmpc::bit>)
        {
            return bit_xor(false_value, bit_and(limb_traits::mask_from(choice), bit_xor(false_value, true_value)));
        }
        else if constexpr (hmpc::is_constant<FalseValue> and hmpc::is_constant<TrueValue>)
        {
            if constexpr (false_value.value == true_value.value)
            {
                return false_value;
            }
            else
            {
                if consteval
                {
                    return bit_xor(false_value, bit_and(limb_traits::mask_from(choice), bit_xor(false_value, true_value)));
                }
                else
                {
                    limb_type x = false_value;
                    limb_type y = true_value;
                    return limb_type{sycl::select(x.data, y.data, choice.data)};
                }
            }
        }
        else
        {
            if consteval
            {
                return bit_xor(false_value, bit_and(limb_traits::mask_from(choice), bit_xor(false_value, true_value)));
            }
            else
            {
                limb_type x = false_value;
                limb_type y = true_value;
                return limb_type{sycl::select(x.data, y.data, choice.data)};
            }
        }
    }
}
