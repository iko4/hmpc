#pragma once

#include <hmpc/access.hpp>
#include <hmpc/constants.hpp>
#include <hmpc/core/limb_size.hpp>

namespace hmpc::core
{
    template<typename Limb, hmpc::size Bits = std::numeric_limits<hmpc::size>::digits, typename Access = hmpc::access::read_tag>
    struct size_limb_span
    {
        using limb_type = Limb;
        using access_type = Access;
        using normal_type = hmpc::access::unnormal_tag;

        static constexpr hmpc::size bit_size = Bits;
        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);

        hmpc::size value;

        constexpr size_limb_span(hmpc::size value) noexcept
            : value(value)
        {
        }

        // TODO: consider signed size_type
        static_assert(not std::numeric_limits<hmpc::size>::is_signed);
        static constexpr hmpc::signedness signedness = hmpc::without_sign;

        constexpr auto sign() const noexcept
        {
            return hmpc::constants::bit::zero;
        }

        constexpr auto sign_mask() const noexcept
        {
            return hmpc::zero_constant_of<limb_type>;
        }

        template<hmpc::size Index, typename Normalization = normal_type>
        constexpr limb_type read(hmpc::size_constant<Index> index = {}, Normalization = {}) const HMPC_DEVICE_NOEXCEPT
            requires hmpc::access::is_read<access_type>
        {
            // TODO: consider normalization
            static_assert(index >= 0);
            static_assert(index < limb_size);
            constexpr hmpc::size shift = index * limb_bit_size;
            return static_cast<limb_type>(value >> shift);
        }

        template<hmpc::size Index, typename Normalization = normal_type>
        constexpr limb_type extended_read(hmpc::size_constant<Index> index = {}, Normalization normal = {}) const HMPC_DEVICE_NOEXCEPT
            requires hmpc::access::is_read<access_type>
        {
            static_assert(index >= 0);
            if constexpr (index < limb_size)
            {
                return read(index, normal);
            }
            else
            {
                return hmpc::zero_constant_of<limb_type>;
            }
        }

        template<hmpc::access::can_access<hmpc::size const&> OtherAccess>
        constexpr auto span(OtherAccess) const noexcept
        {
            return size_limb_span<limb_type, bit_size, OtherAccess>{value};
        }
    };
}
