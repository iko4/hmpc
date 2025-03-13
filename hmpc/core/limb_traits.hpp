#pragma once

#include <hmpc/constant.hpp>
#include <hmpc/core/uint.hpp>

namespace hmpc::core
{
    template<typename Limb>
    struct limb_traits
    {
        using type = Limb;
        static constexpr hmpc::constant<type, type{}> zero = {};
        static constexpr hmpc::constant<type, type{1}> one = {};
        static constexpr hmpc::constant<type, type{}> min = {};
        static constexpr hmpc::constant<type, compl type{}> max = {};

        static constexpr auto all_zeros = zero;
        static constexpr auto all_ones = max;

        static constexpr auto bit_size = type::bit_size;

        static constexpr type value_from(hmpc::bit bit) HMPC_NOEXCEPT
        {
            return type{bit};
        }
        template<hmpc::bit Bit>
        static constexpr auto value_from(hmpc::bit_constant<Bit>) HMPC_NOEXCEPT
        {
            return hmpc::constant_of<value_from(Bit)>;
        }

        template<hmpc::maybe_constant_of<type> Value>
        static constexpr auto value_from(Value value) HMPC_NOEXCEPT
        {
            return value;
        }

        static constexpr type mask_from(hmpc::bit bit) HMPC_NOEXCEPT
        {
            return -type{bit};
        }
        template<hmpc::bit Bit>
        static constexpr auto mask_from(hmpc::bit_constant<Bit>) HMPC_NOEXCEPT
        {
            return hmpc::constant_of<mask_from(Bit)>;
        }

        static_assert(min == zero);
        static_assert(max == -one.value);
        static_assert(all_zeros == zero);
        static_assert(all_ones == max);
    };

    template<>
    struct limb_traits<hmpc::size>
    {
        using type = hmpc::size;
        static constexpr hmpc::constant<type, type{}> zero = {};
        static constexpr hmpc::constant<type, type{1}> one = {};
        static constexpr hmpc::constant<type, std::numeric_limits<type>::min()> min = {};
        static constexpr hmpc::constant<type, std::numeric_limits<type>::max()> max = {};

        static constexpr auto all_zeros = zero;
        static constexpr hmpc::constant<type, -type{1}> all_ones = {};

        static constexpr auto bit_size = hmpc::size_constant_of<std::numeric_limits<type>::digits>;

        static constexpr type value_from(hmpc::bit bit) HMPC_NOEXCEPT
        {
            return static_cast<type>(bit);
        }
        template<hmpc::bit Bit>
        static constexpr auto value_from(hmpc::bit_constant<Bit>) HMPC_NOEXCEPT
        {
            return hmpc::constant_of<value_from(Bit)>;
        }
        template<hmpc::maybe_constant_of<type> Value>
        static constexpr auto value_from(Value value) HMPC_NOEXCEPT
        {
            return value;
        }
        static constexpr type mask_from(hmpc::bit bit) HMPC_NOEXCEPT
        {
            return -value_from(bit);
        }
        template<hmpc::bit Bit>
        static constexpr auto mask_from(hmpc::bit_constant<Bit>) HMPC_NOEXCEPT
        {
            return hmpc::constant_of<mask_from(Bit)>;
        }
    };

    namespace traits
    {
        template<typename T>
        struct extended_limb_type
        {
        };

#if defined(HMPC_HAS_UINT8) and defined(HMPC_HAS_UINT16)
        template<>
        struct extended_limb_type<hmpc::core::uint8>
        {
            using type = hmpc::core::uint16;
        };
#endif
#if defined(HMPC_HAS_UINT16) and defined(HMPC_HAS_UINT32)
        template<>
        struct extended_limb_type<hmpc::core::uint16>
        {
            using type = hmpc::core::uint32;
        };
#endif
#if defined(HMPC_HAS_UINT32) and defined(HMPC_HAS_UINT64)
        template<>
        struct extended_limb_type<hmpc::core::uint32>
        {
            using type = hmpc::core::uint64;
        };
#endif

        template<typename T>
        using extended_limb_type_t = extended_limb_type<T>::type;
    }
}
