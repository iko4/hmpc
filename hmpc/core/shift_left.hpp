#pragma once

#include <hmpc/constant.hpp>

namespace hmpc::core
{
    template<typename Limb, hmpc::size Shift>
    constexpr auto shift_left(Limb value, hmpc::size_constant<Shift> shift) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Limb>)
        {
            return hmpc::constant_of<(value.value << shift)>;
        }
        else
        {
            return value << shift;
        }
    }

    template<typename Limb>
    consteval auto shift_left(Limb value, hmpc::size shift)
    {
        return value << shift;
    }
}
