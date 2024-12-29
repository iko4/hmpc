#pragma once

#include <hmpc/core/limb_span.hpp>

namespace hmpc::core
{
    template<hmpc::size Size, typename Limb>
    struct limb_array
    {
        using limb_type = Limb;

        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = Size;

        limb_type data[limb_size];

        template<typename Self>
        constexpr auto&& operator[](this Self&& self, hmpc::size i) HMPC_NOEXCEPT
        {
            return std::forward<Self>(self).data[i];
        }

        template<typename Self, hmpc::access::can_access<Self> Access = hmpc::access::traits::access_like_t<Self>>
        constexpr auto span(this Self&& self, Access = {}) HMPC_NOEXCEPT
        {
            return limb_span<limb_size, limb_type, Access>{self.data};
        }
    };
}
