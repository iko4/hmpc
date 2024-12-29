#pragma once

#include <hmpc/access.hpp>
#include <hmpc/constants.hpp>
#include <hmpc/detail/utility.hpp>
#include <hmpc/core/is_normal.hpp>
#include <hmpc/core/limb_size.hpp>

namespace hmpc
{
    namespace detail
    {
        template<typename Span>
        concept compiletime_bit_span_when_readable = requires(Span span, hmpc::size i)
        {
            span.sign();
            span.sign_mask();
            span.read(i);
            span.read(i, hmpc::access::normal);
            span.read(i, hmpc::access::unnormal);
            span.extended_read(i);
            span.extended_read(i, hmpc::access::normal);
            span.extended_read(i, hmpc::access::unnormal);
        };

        template<typename Span>
        concept compiletime_bit_span_when_writable = requires(Span span, typename Span::limb_type value, hmpc::size i)
        {
            span.write(i, value);
            span.write(i, value, hmpc::access::normal);
            span.write(i, value, hmpc::access::unnormal);
        };
    }

    template<typename Span>
    concept compiletime_bit_span = requires(Span span)
    {
        typename Span::limb_type;
        typename Span::access_type;
        typename Span::normal_type;
        span.bit_size;
        span.limb_size;
        Span::limb_bit_size;
        Span::signedness;
    };

    template<typename Span>
    concept read_only_compiletime_bit_span = compiletime_bit_span<Span> and hmpc::access::is_read_only<typename Span::access_type> and detail::compiletime_bit_span_when_readable<Span>;

    template<typename Span>
    concept write_only_compiletime_bit_span = compiletime_bit_span<Span> and hmpc::access::is_write_only<typename Span::access_type> and detail::compiletime_bit_span_when_writable<Span>;

    template<typename Span>
    concept read_write_compiletime_bit_span = compiletime_bit_span<Span> and hmpc::access::is_read_write<typename Span::access_type> and detail::compiletime_bit_span_when_readable<Span> and detail::compiletime_bit_span_when_writable<Span>;
}

namespace hmpc::core
{
    template<typename Limb, typename Access, typename Normalization>
    struct compiletime_bit_span
    {
        using limb_type = Limb;
        using access_type = Access;
        using normal_type = Normalization;
        using pointer_type = hmpc::access::traits::pointer_t<limb_type, access_type>;
        using reference_type = hmpc::access::traits::reference_t<limb_type, access_type>;

        static constexpr hmpc::signedness signedness = hmpc::without_sign;
        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;

        hmpc::size bit_size;
        hmpc::size limb_size;
        pointer_type data;

        consteval compiletime_bit_span(hmpc::size bit_size, pointer_type data)
            : bit_size(bit_size)
            , limb_size(hmpc::core::limb_size_for<limb_type>(bit_size))
            , data(data)
        {
        }

        consteval hmpc::size last_limb_bit_size() const
        {
            return (bit_size % limb_bit_size) + ((bit_size % limb_bit_size == 0) and bit_size != 0) * limb_bit_size;
        }

        constexpr auto sign() const HMPC_NOEXCEPT
        {
            return hmpc::constants::bit::zero;
        }

        constexpr auto sign_mask() const HMPC_NOEXCEPT
        {
            return hmpc::zero_constant_of<limb_type>;
        }

        template<typename OtherNormalization = normal_type>
        consteval limb_type read(hmpc::size index, OtherNormalization = {}) const
        {
            static_assert(hmpc::access::is_read<access_type>);
            HMPC_COMPILETIME_ASSERT(index >= 0);
            HMPC_COMPILETIME_ASSERT(index < limb_size);
            HMPC_COMPILETIME_ASSERT(data != nullptr);

            if ((bit_size % limb_bit_size) != 0 and index == limb_size - 1)
            {
                if constexpr (hmpc::access::is_normal<normal_type>)
                {
                    HMPC_COMPILETIME_ASSERT(hmpc::core::is_normal(data[index], last_limb_bit_size()));
                    return data[index];
                }
                else if constexpr (hmpc::access::is_normal<OtherNormalization>)
                {
                    return hmpc::core::mask_inside(data[index], last_limb_bit_size());
                }
                else
                {
                    return data[index];
                }
            }
            else
            {
                return data[index];
            }
        }

        template<typename OtherNormalization = normal_type>
        consteval limb_type extended_read(hmpc::size index, OtherNormalization normal = {}) const
        {
            static_assert(hmpc::access::is_read<access_type>);
            HMPC_COMPILETIME_ASSERT(index >= 0);

            if (index < limb_size)
            {
                return read(index, normal);
            }
            else
            {
                return {};
            }
        }

        template<typename OtherNormalization = normal_type, hmpc::maybe_constant_of<limb_type> Value>
        consteval void write(hmpc::size index, Value value, OtherNormalization = {}) const
        {
            static_assert(hmpc::access::is_write<access_type>);
            HMPC_COMPILETIME_ASSERT(index >= 0);
            HMPC_COMPILETIME_ASSERT(index < limb_size);
            HMPC_COMPILETIME_ASSERT(data != nullptr);

            if (index == limb_size - 1)
            {
                if constexpr (hmpc::access::is_normal<normal_type> and hmpc::access::is_unnormal<OtherNormalization>)
                {
                    HMPC_COMPILETIME_ASSERT(hmpc::core::is_normal(value, last_limb_bit_size()));
                    // fallthrough
                }
                else if constexpr (hmpc::access::is_normal<OtherNormalization>)
                {
                    data[index] = hmpc::core::mask_inside(value, last_limb_bit_size());
                    return;
                }
                // else: fallthrough
            }

            data[index] = value;
        }

        template<hmpc::access::can_access<reference_type> OtherAccess>
        constexpr auto compiletime_span(OtherAccess) const noexcept
        {
            return compiletime_bit_span<limb_type, OtherAccess, normal_type>{bit_size, data};
        }

        consteval auto first_limbs(hmpc::size subspan_length) const
        {
            HMPC_COMPILETIME_ASSERT(subspan_length >= 0);
            if (subspan_length * limb_bit_size < bit_size)
            {
                return compiletime_bit_span{subspan_length * limb_bit_size, data};
            }
            else
            {
                return *this;
            }
        }

        consteval auto subspan(hmpc::size offset, hmpc::size subspan_length = hmpc::dynamic_extent) const
        {
            HMPC_COMPILETIME_ASSERT(offset >= 0);
            if (subspan_length == hmpc::dynamic_extent)
            {
                if (offset * limb_bit_size < bit_size)
                {
                    return compiletime_bit_span{bit_size - offset * limb_bit_size, data + offset};
                }
                else
                {
                    return compiletime_bit_span{0, nullptr};
                }
            }
            else
            {
                HMPC_COMPILETIME_ASSERT(subspan_length >= 0);
                HMPC_COMPILETIME_ASSERT(limb_size >= offset + subspan_length);
                if (subspan_length == 0)
                {
                    return compiletime_bit_span{0, nullptr};
                }
                HMPC_COMPILETIME_ASSERT(bit_size >= offset * limb_bit_size);
                if (subspan_length * limb_bit_size < bit_size - offset * limb_bit_size)
                {
                    return compiletime_bit_span{subspan_length * limb_bit_size, data + offset};
                }
                else
                {
                    return compiletime_bit_span{bit_size - offset * limb_bit_size, data + offset};
                }
            }
        }
    };

    template<typename Limb, typename Access = hmpc::access::read_write_tag, typename Normalization = hmpc::access::unnormal_tag>
    constexpr auto compiletime_nullspan = compiletime_bit_span<Limb, Access, Normalization>{0, nullptr};

    template<typename Limb, typename Normalization = hmpc::access::unnormal_tag>
    constexpr auto read_compiletime_nullspan = compiletime_nullspan<Limb, hmpc::access::read_tag, Normalization>;

    template<typename Limb, typename Normalization = hmpc::access::unnormal_tag>
    constexpr auto write_compiletime_nullspan = compiletime_nullspan<Limb, hmpc::access::write_tag, Normalization>;
}
