#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/select.hpp>

namespace hmpc::core
{
    template<hmpc::read_only_bit_span FalseSpan, hmpc::read_only_bit_span TrueSpan>
        requires (hmpc::same_limb_types<FalseSpan, TrueSpan>)
    struct select_bit_span
    {
        using false_bit_span = FalseSpan;
        using true_bit_span = TrueSpan;
        using limb_type = false_bit_span::limb_type;
        using access_type = hmpc::access::traits::common_access_t<typename false_bit_span::access_type, typename true_bit_span::access_type>;
        using normal_type = hmpc::access::traits::common_normal_t<typename false_bit_span::normal_type, typename true_bit_span::normal_type>;

        static_assert(hmpc::access::is_read_only<access_type>);

        static constexpr hmpc::size bit_size = std::max(false_bit_span::bit_size, true_bit_span::bit_size);
        static constexpr hmpc::signedness signedness = hmpc::common_signedness(false_bit_span::signedness, true_bit_span::signedness);
        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = std::max(false_bit_span::limb_size, true_bit_span::limb_size);

        /// Data members
        [[no_unique_address]] false_bit_span false_data;
        [[no_unique_address]] true_bit_span true_data;
        [[no_unique_address]] hmpc::bit HMPC_PRIVATE_MEMBER(choice);

        constexpr select_bit_span(false_bit_span false_data, true_bit_span true_data, hmpc::bit choice) HMPC_NOEXCEPT
            : false_data(false_data)
            , true_data(true_data)
            , HMPC_PRIVATE_MEMBER(choice)(choice)
        {
        }

        constexpr auto choice() const HMPC_NOEXCEPT
        {
            return HMPC_PRIVATE_MEMBER(choice);
        }

        constexpr auto sign() const HMPC_NOEXCEPT
        {
            return hmpc::core::select(false_data.sign(), true_data.sign(), HMPC_PRIVATE_MEMBER(choice));
        }

        constexpr auto sign_mask() const HMPC_NOEXCEPT
        {
            return hmpc::core::select(false_data.sign_mask(), true_data.sign_mask(), HMPC_PRIVATE_MEMBER(choice));
        }

        template<hmpc::size Index, typename OtherNormalization = normal_type>
        constexpr limb_type read(hmpc::size_constant<Index> index = {}, OtherNormalization normal = {}) const HMPC_NOEXCEPT
        {
            static_assert(index >= 0);
            static_assert(index < limb_size);
            return hmpc::core::select(false_data.extended_read(index, normal), true_data.extended_read(index, normal), HMPC_PRIVATE_MEMBER(choice));
        }

        template<hmpc::size Index, typename OtherNormalization = normal_type>
        constexpr auto extended_read(hmpc::size_constant<Index> index = {}, OtherNormalization normal = {}) const HMPC_NOEXCEPT
        {
            return hmpc::core::select(false_data.extended_read(index, normal), true_data.extended_read(index, normal), HMPC_PRIVATE_MEMBER(choice));
        }

        constexpr select_bit_span span(access_type) const noexcept
        {
            return *this;
        }
    };
}
