#pragma once

#include <hmpc/detail/utility.hpp>

namespace hmpc::core
{
    template<typename Limb>
    constexpr hmpc::size limb_size_for(hmpc::size bit_size, hmpc::size limb_bit_size = Limb::bit_size) noexcept
    {
        return hmpc::detail::div_ceil(bit_size, limb_bit_size);
    }
}
