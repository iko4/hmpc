#pragma once

#include <hmpc/core/extract_bit.hpp>
#include <hmpc/core/limb_size.hpp>
#include <hmpc/core/limb_traits.hpp>

namespace hmpc::core
{
    template<hmpc::size Bits, typename Limb, hmpc::signedness Signedness, typename Access>
    struct bit_span_metadata
    {
        using limb_type = Limb;
        using access_type = Access;
        using pointer_type = hmpc::access::traits::pointer_t<limb_type, access_type>;
        using reference_type = hmpc::access::traits::reference_t<limb_type, access_type>;

        static constexpr hmpc::size bit_size = Bits;

        static constexpr hmpc::signedness signedness = Signedness;

        /// Data member
        pointer_type pointer;

        constexpr bit_span_metadata(pointer_type pointer) HMPC_NOEXCEPT
            : pointer(pointer)
        {
        }

        template<hmpc::size OtherBits, hmpc::access::can_access<reference_type> OtherAccess>
        constexpr bit_span_metadata(bit_span_metadata<OtherBits, limb_type, signedness, OtherAccess> other) HMPC_NOEXCEPT
            : pointer(other.pointer)
        {
            static_assert(bit_size <= OtherBits);
        }

        constexpr pointer_type ptr() const
        {
            return pointer;
        }

        constexpr auto sign() const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type>);
            if constexpr (hmpc::is_signed(signedness))
            {
                constexpr hmpc::size limb_bit_size = limb_type::bit_size;
                constexpr hmpc::size limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);
                static_assert(bit_size > 0);
                static_assert(limb_size > 0);
                HMPC_DEVICE_ASSERT(pointer != nullptr);
                return extract_bit<(bit_size - 1) % limb_bit_size>(pointer[limb_size - 1]);
            }
            else
            {
                return hmpc::constants::bit::zero;
            }
        }

        constexpr auto sign_mask() const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type>);
            if constexpr (hmpc::is_signed(signedness))
            {
                using limb_traits = hmpc::core::limb_traits<limb_type>;
                constexpr hmpc::size limb_bit_size = limb_type::bit_size;
                constexpr hmpc::size limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);
                static_assert(bit_size > 0);
                static_assert(limb_size > 0);
                HMPC_DEVICE_ASSERT(pointer != nullptr);
                return limb_traits::mask_from(hmpc::core::extract_bit<(bit_size - 1) % limb_bit_size>(pointer[limb_size - 1]));
            }
            else
            {
                return hmpc::zero_constant_of<limb_type>;
            }
        }

        constexpr reference_type operator[](hmpc::size index) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type> or hmpc::access::is_write<access_type>);
            return pointer[index];
        }

        constexpr bool operator!=(std::nullptr_t) const HMPC_NOEXCEPT
        {
            return pointer != nullptr;
        }

        constexpr bit_span_metadata operator+(hmpc::size offset) const HMPC_NOEXCEPT
        {
            return {pointer + offset};
        }
    };

    template<hmpc::size Bits, typename Limb>
    struct bit_span_metadata<Bits, Limb, hmpc::with_sign, hmpc::access::read_tag>
    {
        using limb_type = Limb;
        using access_type = hmpc::access::read_tag;
        using pointer_type = hmpc::access::traits::pointer_t<limb_type, access_type>;
        using reference_type = hmpc::access::traits::reference_t<limb_type, access_type>;

        static constexpr hmpc::size bit_size = Bits;

        static constexpr hmpc::signedness signedness = hmpc::with_sign;

        /// Data members
        pointer_type pointer;
        [[no_unique_address]] hmpc::bit HMPC_PRIVATE_MEMBER(sign);

        constexpr bit_span_metadata(pointer_type pointer) HMPC_NOEXCEPT
            : bit_span_metadata(pointer, (HMPC_DEVICE_ASSERT(pointer != nullptr), hmpc::core::extract_bit<(bit_size - 1) % limb_type::bit_size>(pointer[hmpc::core::limb_size_for<limb_type>(bit_size) - 1])))
        {
            constexpr hmpc::size limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);
            static_assert(bit_size > 0);
            static_assert(limb_size > 0);
        }

        constexpr bit_span_metadata(pointer_type pointer, hmpc::bit sign) HMPC_NOEXCEPT
            : pointer(pointer)
            , HMPC_PRIVATE_MEMBER(sign)(sign)
        {
        }

        constexpr pointer_type ptr() const
        {
            return pointer;
        }

        constexpr hmpc::bit sign() const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type>);
            return HMPC_PRIVATE_MEMBER(sign);
        }

        constexpr limb_type sign_mask() const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type>);
            using limb_traits = hmpc::core::limb_traits<limb_type>;
            return limb_traits::mask_from(HMPC_PRIVATE_MEMBER(sign));
        }

        constexpr reference_type operator[](hmpc::size index) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type> or hmpc::access::is_write<access_type>);
            return pointer[index];
        }

        constexpr bool operator!=(std::nullptr_t) const HMPC_NOEXCEPT
        {
            return pointer != nullptr;
        }
    };
}
