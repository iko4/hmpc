#pragma once

#include <hmpc/constant.hpp>

namespace hmpc::core
{
    template<typename Limb>
    constexpr auto bit_not(Limb value) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Limb>)
        {
            return hmpc::constant_of<compl value.value>;
        }
        else
        {
            return compl value;
        }
    }
}
