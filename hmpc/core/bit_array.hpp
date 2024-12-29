#pragma once

#include <hmpc/core/bit_span.hpp>

namespace hmpc::core
{
    template<hmpc::size Bits, typename Limb, hmpc::signedness Signedness, typename Normalization = hmpc::access::unnormal_tag>
    struct bit_array
    {
        using limb_type = Limb;
        using normal_type = Normalization;

        static constexpr hmpc::size bit_size = Bits;

        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);

        static constexpr hmpc::signedness signedness = Signedness;

        /// Data member
        limb_type data[limb_size];

        template<typename Self>
        constexpr auto&& operator[](this Self&& self, hmpc::size i) HMPC_NOEXCEPT
        {
            return std::forward<Self>(self).data[i];
        }

        template<typename Self, hmpc::access::can_access<Self> Access = hmpc::access::traits::access_like_t<Self>>
        constexpr auto span(this Self&& self, Access = {}) HMPC_NOEXCEPT
        {
            return bit_span<bit_size, limb_type, signedness, Access, normal_type>{self.data};
        }

        template<typename Self, hmpc::access::can_access<Self> Access = hmpc::access::traits::access_like_t<Self>>
        consteval auto compiletime_span(this Self&& self, Access = {})
        {
            static_assert(hmpc::is_unsigned(signedness));
            return compiletime_bit_span<limb_type, Access, normal_type>{bit_size, self.data};
        }
    };

    template<typename Limb, hmpc::signedness Signedness, typename Normalization>
    struct bit_array<0, Limb, Signedness, Normalization>
    {
        using limb_type = Limb;
        using normal_type = Normalization;

        static constexpr hmpc::size bit_size = 0;

        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = 0;

        static constexpr hmpc::signedness signedness = Signedness;

        /// No data member

        template<typename Self, hmpc::access::can_access<Self> Access = hmpc::access::traits::access_like_t<Self>>
        constexpr auto span(this Self&&, Access = {}) HMPC_NOEXCEPT
        {
            return bit_span<bit_size, limb_type, signedness, Access, normal_type>{nullptr};
        }
    };
}
