#pragma once

#include <hmpc/access.hpp>
#include <hmpc/constants.hpp>
#include <hmpc/core/bit_not.hpp>
#include <hmpc/core/bit_span_metadata.hpp>
#include <hmpc/core/bit_width.hpp>
#include <hmpc/core/cast.hpp>
#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/core/extend_sign.hpp>
#include <hmpc/core/extract_bit.hpp>
#include <hmpc/core/is_normal.hpp>
#include <hmpc/core/limb_span.hpp>
#include <hmpc/detail/constant_list.hpp>
#include <hmpc/detail/utility.hpp>
#include <hmpc/iter/scan_reverse_range.hpp>

#include <format>

namespace hmpc
{
    namespace detail
    {
        template<typename Span>
        concept bit_span_when_readable = limb_span_when_readable<Span> and requires(Span span)
        {
            span.sign();
            span.sign_mask();
            span.extended_read(hmpc::constants::zero, hmpc::access::normal);
            span.extended_read(hmpc::constants::zero, hmpc::access::unnormal);
        };

        template<typename Span>
        concept bit_span_when_writable = limb_span_when_writable<Span> and ((Span::limb_size <= 0) or requires(Span span, typename Span::limb_type value)
        {
            span.write(hmpc::constants::zero, value, hmpc::access::normal);
            span.write(hmpc::constants::zero, value, hmpc::access::unnormal);
        });
    }

    template<typename Span>
    concept bit_span = limb_span<Span> and requires
    {
        typename Span::normal_type;
        Span::bit_size;
        Span::signedness;
    };

    template<typename Span>
    concept read_only_bit_span = bit_span<Span> and read_only_limb_span<Span> and detail::bit_span_when_readable<Span>;

    template<typename Span>
    concept write_only_bit_span = bit_span<Span> and write_only_limb_span<Span> and detail::bit_span_when_writable<Span>;

    template<typename Span>
    concept read_write_bit_span = bit_span<Span> and read_write_limb_span<Span> and detail::bit_span_when_readable<Span> and detail::bit_span_when_writable<Span>;

    template<typename Span>
    concept unsigned_read_only_bit_span = read_only_bit_span<Span> and hmpc::is_unsigned(Span::signedness);

    template<typename Span>
    concept unsigned_write_only_bit_span = write_only_bit_span<Span> and hmpc::is_unsigned(Span::signedness);

    template<typename Span>
    concept unsigned_read_write_bit_span = read_write_bit_span<Span> and hmpc::is_unsigned(Span::signedness);
}

namespace hmpc::core
{
    template<hmpc::read_only_bit_span Span>
    struct masked_bit_span;

    template<hmpc::size Bits, typename Limb, hmpc::signedness Signedness, typename Access, typename Normalization>
    struct bit_span
    {
        using limb_type = Limb;
        using access_type = Access;
        using normal_type = Normalization;
        using pointer_type = hmpc::access::traits::pointer_t<limb_type, access_type>;
        using reference_type = hmpc::access::traits::reference_t<limb_type, access_type>;

        static constexpr auto bit_size = hmpc::size_constant_of<Bits>;

        static constexpr hmpc::signedness signedness = Signedness;

        static constexpr auto limb_bit_size = limb_type::bit_size;
        static constexpr auto last_limb_bit_size = hmpc::size_constant_of<(bit_size % limb_bit_size) + ((bit_size % limb_bit_size == 0) and bit_size != 0) * limb_bit_size>;
        static constexpr auto limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);

        /// Data member
        bit_span_metadata<bit_size, limb_type, signedness, access_type> data;

        constexpr pointer_type ptr() const
        {
            return data.ptr();
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

        constexpr auto sign() const HMPC_NOEXCEPT
        {
            return data.sign();
        }

        constexpr auto sign_mask() const HMPC_NOEXCEPT
        {
            return data.sign_mask();
        }

        template<hmpc::size Index, typename OtherNormalization = normal_type>
        constexpr limb_type read(hmpc::size_constant<Index> index = {}, OtherNormalization = {}) const HMPC_NOEXCEPT
        {
            static_assert(index >= 0);
            static_assert(index < limb_size);
            static_assert(hmpc::access::is_read<access_type>);
            HMPC_DEVICE_ASSERT(data != nullptr);
            if constexpr ((bit_size % limb_bit_size) != 0 and index == limb_size - 1)
            {
                if constexpr (hmpc::access::is_normal<normal_type>)
                {
                    HMPC_DEVICE_ASSERT(hmpc::core::is_normal<last_limb_bit_size>(data[index], data.sign_mask()));
                    return data[index];
                }
                else if (hmpc::access::is_normal<OtherNormalization>)
                {
                    if constexpr (hmpc::is_signed(signedness))
                    {
                        return hmpc::core::extend_sign<last_limb_bit_size>(data[index], data.sign_mask());
                    }
                    else
                    {
                        return hmpc::core::mask_inside<last_limb_bit_size>(data[index]);
                    }
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

        template<hmpc::size Index, typename OtherNormalization = normal_type>
        constexpr auto extended_read(hmpc::size_constant<Index> index = {}, OtherNormalization normal = {}) const HMPC_NOEXCEPT
        {
            static_assert(index >= 0);
            if constexpr (index < limb_size)
            {
                return read(index, normal);
            }
            else
            {
                return data.sign_mask();
            }
        }

        template<hmpc::size Index, typename OtherNormalization = normal_type, hmpc::maybe_constant_of<limb_type> Value>
        constexpr void write(hmpc::size_constant<Index> index, Value value, OtherNormalization = {}) const HMPC_NOEXCEPT
        {
            static_assert(index >= 0);
            static_assert(index < limb_size);
            static_assert(hmpc::access::is_write<access_type>);
            HMPC_DEVICE_ASSERT(data != nullptr);
            if constexpr (index == limb_size - 1)
            {
                if constexpr (hmpc::access::is_normal<normal_type> and hmpc::access::is_unnormal<OtherNormalization>)
                {
                    if constexpr (hmpc::is_signed(signedness))
                    {
                        HMPC_DEVICE_ASSERT(hmpc::core::is_normal<last_limb_bit_size>(value, hmpc::core::extract_bit<(bit_size - 1) % limb_bit_size>(value)));
                        // fallthrough
                    }
                    else
                    {
                        HMPC_DEVICE_ASSERT(hmpc::core::is_normal<last_limb_bit_size>(value));
                        // fallthrough
                    }
                }
                else if constexpr (hmpc::access::is_normal<OtherNormalization>)
                {
                    if constexpr (hmpc::is_signed(signedness))
                    {
                        auto sign = hmpc::core::extract_bit<(bit_size - 1) % limb_bit_size>(value);
                        data[index] = hmpc::core::extend_sign<last_limb_bit_size>(value, sign);
                        return;
                    }
                    else
                    {
                        data[index] = hmpc::core::mask_inside<last_limb_bit_size>(value);
                        return;
                    }
                }
                // else: fallthrough
            }

            data[index] = value;
        }

        template<hmpc::access::can_access<reference_type> OtherAccess>
        constexpr auto span(OtherAccess) const noexcept
        {
            return bit_span<bit_size, limb_type, signedness, OtherAccess, normal_type>{data};
        }

        template<hmpc::size SubspanLength>
        constexpr auto first_bits(hmpc::size_constant<SubspanLength> subspan_length = {}) const HMPC_NOEXCEPT
            requires (hmpc::is_unsigned(signedness))
        {
            static_assert(subspan_length >= 0);
            static_assert(bit_size >= subspan_length);
            return bit_span<subspan_length, limb_type, signedness, access_type, normal_type>{data};
        }

        template<hmpc::size SubspanLength>
        constexpr auto first_limbs(hmpc::size_constant<SubspanLength> subspan_length = {}) const HMPC_NOEXCEPT
            requires (hmpc::is_unsigned(signedness))
        {
            static_assert(subspan_length >= 0);
            static_assert(limb_size >= subspan_length);
            if constexpr (subspan_length * limb_bit_size < bit_size)
            {
                return bit_span<subspan_length * limb_bit_size, limb_type, signedness, access_type, normal_type>{data};
            }
            else
            {
                return *this;
            }
        }

        template<hmpc::size SubspanLength>
        constexpr auto last_limbs(hmpc::size_constant<SubspanLength> subspan_length = {}) const HMPC_NOEXCEPT
            requires (hmpc::is_unsigned(signedness))
        {
            static_assert(subspan_length >= 0);
            static_assert(limb_size >= subspan_length);
            if constexpr (subspan_length == 0)
            {
                return bit_span<0, limb_type, signedness, access_type, normal_type>{nullptr};
            }
            else
            {
                constexpr hmpc::size offset = limb_size - subspan_length;
                static_assert(bit_size >= offset * limb_bit_size);
                return bit_span<bit_size - offset * limb_bit_size, limb_type, signedness, access_type, normal_type>{data + offset};
            }
        }

        template<hmpc::size Offset, hmpc::size SubspanLength = hmpc::dynamic_extent>
        constexpr auto subspan(hmpc::size_constant<Offset> offset = {}, hmpc::size_constant<SubspanLength> subspan_length = {}) const HMPC_NOEXCEPT
            requires (hmpc::is_unsigned(signedness))
        {
            static_assert(offset >= 0);
            if constexpr (subspan_length == hmpc::dynamic_extent)
            {
                static_assert(limb_size >= offset);
                if constexpr (offset * limb_bit_size < bit_size)
                {
                    return bit_span<bit_size - offset * limb_bit_size, limb_type, signedness, access_type, normal_type>{data + offset};
                }
                else
                {
                    return bit_span<0, limb_type, signedness, access_type, normal_type>{nullptr};
                }
            }
            else
            {
                static_assert(subspan_length >= 0);
                static_assert(limb_size >= offset + subspan_length);
                if constexpr (subspan_length == 0)
                {
                    return bit_span<0, limb_type, signedness, access_type, normal_type>{nullptr};
                }
                static_assert(bit_size >= offset * limb_bit_size);
                if constexpr (subspan_length * limb_bit_size < bit_size - offset * limb_bit_size)
                {
                    return bit_span<subspan_length * limb_bit_size, limb_type, signedness, access_type, normal_type>{data + offset};
                }
                else
                {
                    return bit_span<bit_size - offset * limb_bit_size, limb_type, signedness, access_type, normal_type>{data + offset};
                }
            }
        }

        constexpr auto mask(hmpc::bit mask) const HMPC_NOEXCEPT
            requires (hmpc::access::is_read<access_type>)
        {
            return masked_bit_span{*this, mask};
        }
    };

    template<hmpc::size Bits, typename Limb, typename Access, typename Normalization>
    using unsigned_bit_span = bit_span<Bits, Limb, hmpc::without_sign, Access, Normalization>;

    template<hmpc::size Bits, typename Limb, typename Access, typename Normalization>
    using signed_bit_span = bit_span<Bits, Limb, hmpc::with_sign, Access, Normalization>;

    template<typename Limb, typename Access = hmpc::access::read_write_tag, typename Normalization = hmpc::access::unnormal_tag>
    constexpr auto nullspan = unsigned_bit_span<0, Limb, Access, Normalization>{nullptr};

    template<typename Limb, typename Normalization = hmpc::access::unnormal_tag>
    constexpr auto read_nullspan = nullspan<Limb, hmpc::access::read_tag, Normalization>;

    template<typename Limb, typename Normalization = hmpc::access::unnormal_tag>
    constexpr auto write_nullspan = nullspan<Limb, hmpc::access::write_tag, Normalization>;
}

template<hmpc::read_only_bit_span Span, typename Char>
struct std::formatter<Span, Char>
{
    static constexpr bool is_specialized = true;

    constexpr auto parse(auto& ctx)
    {
        auto it = ctx.begin();
        for (; (it != ctx.end() and *it != Char{'}'}); ++it)
        {
        }
        return it;
    }

    auto format(Span span, auto& ctx) const
    {
        static_assert(Span::limb_bit_size >= 4);
        static_assert(Span::limb_bit_size % 4 == 0);
        return hmpc::iter::scan_reverse(hmpc::range(Span::limb_size), [&](auto i, auto out)
        {
            return std::format_to(out, "{:0{}x}", span.read(i).data, Span::limb_bit_size / 4);
        }, std::format_to(ctx.out(), "0x"));
    }
};
