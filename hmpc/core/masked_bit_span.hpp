#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/select.hpp>

namespace hmpc::core
{
    template<hmpc::read_only_bit_span Span>
    struct masked_bit_span
    {
        using inner_bit_span = Span;
        using limb_type = inner_bit_span::limb_type;
        using access_type = inner_bit_span::access_type;
        using normal_type = inner_bit_span::normal_type;

        static constexpr auto bit_size = inner_bit_span::bit_size;
        static constexpr auto signedness = inner_bit_span::signedness;
        static constexpr auto limb_bit_size = inner_bit_span::limb_bit_size;
        static constexpr auto limb_size = inner_bit_span::limb_size;

        /// Data members
        [[no_unique_address]] inner_bit_span data;
        [[no_unique_address]] hmpc::bit HMPC_PRIVATE_MEMBER(mask);

        constexpr masked_bit_span(inner_bit_span data, hmpc::bit mask) HMPC_NOEXCEPT
            : data(data)
            , HMPC_PRIVATE_MEMBER(mask)(mask)
        {
        }

        constexpr auto choice() const HMPC_NOEXCEPT
        {
            return HMPC_PRIVATE_MEMBER(mask);
        }

        constexpr auto mask() const HMPC_NOEXCEPT
        {
            return HMPC_PRIVATE_MEMBER(mask);
        }

        constexpr auto sign() const HMPC_NOEXCEPT
        {
            return hmpc::core::select(hmpc::constants::bit::zero, data.sign(), HMPC_PRIVATE_MEMBER(mask));
        }

        constexpr auto sign_mask() const HMPC_NOEXCEPT
        {
            return hmpc::core::select(hmpc::zero_constant_of<limb_type>, data.sign_mask(), HMPC_PRIVATE_MEMBER(mask));
        }

        template<hmpc::size Index, typename OtherNormalization = normal_type>
        constexpr auto read(hmpc::size_constant<Index> index = {}, OtherNormalization normal = {}) const HMPC_NOEXCEPT
        {
            return hmpc::core::select(hmpc::zero_constant_of<limb_type>, data.read(index, normal), HMPC_PRIVATE_MEMBER(mask));
        }

        template<hmpc::size Index, typename OtherNormalization = normal_type>
        constexpr auto extended_read(hmpc::size_constant<Index> index = {}, OtherNormalization normal = {}) const HMPC_NOEXCEPT
        {
            return hmpc::core::select(hmpc::zero_constant_of<limb_type>, data.extended_read(index, normal), HMPC_PRIVATE_MEMBER(mask));
        }

        constexpr masked_bit_span span(access_type) const HMPC_NOEXCEPT
        {
            return *this;
        }

        constexpr auto mask(hmpc::bit other_mask) const HMPC_NOEXCEPT
        {
            return masked_bit_span{data, HMPC_PRIVATE_MEMBER(mask) bitand other_mask};
        }
    };
}
