#pragma once

#include <hmpc/constant.hpp>
#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/num/bit_width.hpp>
#include <hmpc/iter/for_packed_range.hpp>

namespace hmpc::core
{
    template<typename Limb, Limb... Limbs>
    struct constant_bit_span
    {
        using limb_type = Limb;
        using access_type = hmpc::access::read_tag;
        using normal_type = hmpc::access::normal_tag;

        static constexpr hmpc::signedness signedness = hmpc::without_sign;
        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = sizeof...(Limbs);
        static constexpr hmpc::size bit_size = hmpc::iter::for_packed_range<limb_size>([](auto... i)
        {
            if constexpr (limb_size == 0)
            {
                return 0;
            }
            else
            {
                return (((i == limb_size - 1) ? hmpc::core::bit_width(Limbs) : limb_bit_size) + ...);
            }
        });

        constexpr constant_bit_span() noexcept = default;
        constexpr constant_bit_span(hmpc::constant<limb_type, Limbs>...) noexcept
        {
        }

        constexpr auto sign() const noexcept
        {
            return hmpc::constants::bit::zero;
        }

        constexpr auto sign_mask() const noexcept
        {
            return hmpc::zero_constant_of<limb_type>;
        }

        template<hmpc::size Index, typename OtherNormalization = normal_type>
        constexpr auto read(hmpc::size_constant<Index> index = {}, OtherNormalization = {}) const noexcept
        {
            static_assert(index >= 0);
            static_assert(index < limb_size);

            return hmpc::detail::constant_list<limb_type, Limbs...>::get(index);
        }

        template<hmpc::size Index, typename OtherNormalization = normal_type>
        constexpr auto extended_read(hmpc::size_constant<Index> index = {}, OtherNormalization normal = {}) const noexcept
        {
            static_assert(index >= 0);
            if constexpr (index < limb_size)
            {
                return read(index, normal);
            }
            else
            {
                return sign_mask();
            }
        }

        constexpr constant_bit_span span(access_type) const noexcept
        {
            return {};
        }

        constexpr auto mask(hmpc::bit mask) const noexcept
        {
            return masked_bit_span{*this, mask};
        }
    };

    template<hmpc::read_only_bit_span auto Span>
    constexpr auto constant_bit_span_from = []()
    {
        using span_type = decltype(Span);
        using limb_type = span_type::limb_type;

        static_assert(hmpc::core::cast<bool>(hmpc::core::bit_not(Span.sign())));

        constexpr auto bit_size = hmpc::core::num::bit_width(Span);
        constexpr auto limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);
        return hmpc::iter::for_packed_range<limb_size>([](auto... i)
        {
            return constant_bit_span{[&]()
            {
                constexpr limb_type x = Span.read(i);
                return hmpc::constant_of<x>;
            }()...};
        });
    }();

    template<auto Value>
    constexpr auto constant_bit_span_of = constant_bit_span_from<Value.span(hmpc::access::read)>;
}
