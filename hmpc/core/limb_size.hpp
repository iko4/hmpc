#pragma once

#include <hmpc/detail/utility.hpp>

namespace hmpc::core
{
    template<typename Limb, hmpc::maybe_constant_of<hmpc::size> Size>
    constexpr auto limb_size_for(Size bit_size) noexcept
    {
        constexpr hmpc::size limb_bit_size = Limb::bit_size;
        if constexpr (hmpc::is_constant<Size>)
        {
            return hmpc::size_constant_of<hmpc::detail::div_ceil(bit_size.value, limb_bit_size)>;
        }
        else
        {
            return hmpc::detail::div_ceil(bit_size, limb_bit_size);
        }
    }
}
