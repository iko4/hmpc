#pragma once

#include <hmpc/access.hpp>
#include <hmpc/constants.hpp>
#include <hmpc/detail/utility.hpp>

namespace hmpc
{
    namespace detail
    {
        template<typename Span>
        concept limb_span_when_readable = requires(Span span)
        {
            span.extended_read(hmpc::constants::zero);
        };

        template<typename Span>
        concept limb_span_when_writable = (Span::limb_size <= 0) or requires(Span span, typename Span::limb_type value)
        {
            span.write(hmpc::constants::zero, value);
        };
    }

    template<typename Span>
    concept limb_span = requires
    {
        typename Span::limb_type;
        typename Span::access_type;
        Span::limb_bit_size;
        Span::limb_size;
    } and std::bool_constant<(Span::limb_bit_size > 0)>::value and std::bool_constant<(Span::limb_size >= 0)>::value;

    template<typename Span>
    concept read_only_limb_span = limb_span<Span> and hmpc::access::is_read_only<typename Span::access_type> and detail::limb_span_when_readable<Span>;

    template<typename Span>
    concept write_only_limb_span = limb_span<Span> and hmpc::access::is_write_only<typename Span::access_type> and detail::limb_span_when_writable<Span>;

    template<typename Span>
    concept read_write_limb_span = limb_span<Span> and hmpc::access::is_read_write<typename Span::access_type> and detail::limb_span_when_readable<Span> and detail::limb_span_when_writable<Span>;

    template<typename Span, typename... Spans>
    concept same_limb_types = (std::same_as<typename Span::limb_type, typename Spans::limb_type> and ...);
}

namespace hmpc::core
{
    template<hmpc::size Size, typename Limb, typename Access>
    struct limb_span
    {
        using limb_type = Limb;
        using access_type = Access;
        using pointer_type = hmpc::access::traits::pointer_t<limb_type, access_type>;
        using reference_type = hmpc::access::traits::reference_t<limb_type, access_type>;

        static constexpr auto limb_bit_size = limb_type::bit_size;
        static constexpr auto limb_size = hmpc::size_constant_of<Size>;

        pointer_type data;

        constexpr limb_span(pointer_type data) HMPC_NOEXCEPT
            : data(data)
        {
        }

        constexpr pointer_type ptr() const
        {
            return data;
        }

        template<hmpc::size I>
        constexpr reference_type operator[](hmpc::size_constant<I> index) const HMPC_NOEXCEPT
        {
            static_assert(index >= 0);
            static_assert(index < limb_size);
            static_assert(hmpc::access::is_read<access_type> or hmpc::access::is_write<access_type>);
            HMPC_DEVICE_ASSERT(data != nullptr);
            return data[index];
        }

        template<hmpc::size I>
        constexpr auto extended_read(hmpc::size_constant<I> index = {}) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type>);
            static_assert(index >= 0);
            if constexpr (index < limb_size)
            {
                HMPC_DEVICE_ASSERT(data != nullptr);
                return data[index];
            }
            else
            {
                return hmpc::zero_constant_of<limb_type>;
            }
        }

        template<hmpc::size I>
        constexpr void write(hmpc::size_constant<I> index, limb_type value) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_write<access_type>);
            static_assert(index >= 0);
            static_assert(index < limb_size);
            HMPC_DEVICE_ASSERT(data != nullptr);
            data[index] = value;
        }

        template<typename OtherAccess>
        constexpr limb_span<limb_size, limb_type, OtherAccess> span(OtherAccess) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::detail::implies(hmpc::access::is_read<OtherAccess>, hmpc::access::is_read<access_type>));
            static_assert(hmpc::detail::implies(hmpc::access::is_write<OtherAccess>, hmpc::access::is_write<access_type>));
            return {data};
        }

        template<hmpc::size Offset, hmpc::size SubspanLength = hmpc::dynamic_extent>
        constexpr auto subspan(hmpc::size_constant<Offset> offset = {}, hmpc::size_constant<SubspanLength> subspan_length = {}) const HMPC_NOEXCEPT
        {
            static_assert(offset >= 0);
            if constexpr (subspan_length == hmpc::dynamic_extent)
            {
                static_assert(limb_size >= offset);
                if constexpr (offset < limb_size)
                {
                    return limb_span<limb_size - offset, limb_type, access_type>{data + offset};
                }
                else
                {
                    return limb_span<0, limb_type, access_type>{nullptr};
                }
            }
            else
            {
                static_assert(subspan_length >= 0);
                static_assert(limb_size >= offset + subspan_length);
                if constexpr (subspan_length == 0)
                {
                    return limb_span<0, limb_type, access_type>{nullptr};
                }
                static_assert(limb_size >= offset);
                if constexpr (subspan_length < limb_size - offset)
                {
                    return limb_span<subspan_length, limb_type, access_type>{data + offset};
                }
                else
                {
                    return limb_span<limb_size - offset, limb_type, access_type>{data + offset};
                }
            }
        }
    };
}
