#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/shift_left.hpp>
#include <hmpc/core/shift_right.hpp>

namespace hmpc::core
{
    template<typename Limb, typename Inner>
    struct converting_limb_span;

    template<typename Limb, hmpc::limb_span Inner>
        requires (Limb::bit_size == Inner::limb_bit_size)
    struct converting_limb_span<Limb, Inner>
    {
        using limb_type = Limb;
        using inner_type = Inner;
        using inner_limb_type = inner_type::limb_type;

        using access_type = inner_type::access_type;

        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size inner_limb_bit_size = inner_type::limb_bit_size;

        static_assert(limb_bit_size == inner_limb_bit_size);

        static constexpr hmpc::size ratio = 1;

        static constexpr hmpc::size inner_limb_size = inner_type::limb_size;
        static constexpr hmpc::size limb_size = inner_limb_size;

        inner_type inner;

        template<hmpc::size I>
        constexpr auto extended_read(hmpc::size_constant<I> index = {}) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type>);
            static_assert(index >= 0);
            if constexpr (index < limb_size)
            {
                return hmpc::core::cast<limb_type>(inner.extended_read(index));
            }
            else
            {
                return hmpc::zero_constant_of<limb_type>;
            }
        }

        template<hmpc::size Index, hmpc::maybe_constant_of<limb_type> Value>
        constexpr void write(hmpc::size_constant<Index> index, Value value) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_write<access_type>);
            static_assert(index >= 0);
            static_assert(index < limb_size);

            inner.write(index, hmpc::core::cast<inner_limb_type>(value));
        }
    };

    template<typename Limb, hmpc::limb_span Inner>
        requires (Limb::bit_size > Inner::limb_bit_size)
    struct converting_limb_span<Limb, Inner>
    {
        using limb_type = Limb;
        using inner_type = Inner;
        using inner_limb_type = inner_type::limb_type;

        using access_type = inner_type::access_type;

        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size inner_limb_bit_size = inner_type::limb_bit_size;

        static_assert(limb_bit_size > inner_limb_bit_size);
        static_assert(limb_bit_size % inner_limb_bit_size == 0);

        static constexpr hmpc::size ratio = limb_bit_size / inner_limb_bit_size;

        static constexpr hmpc::size inner_limb_size = inner_type::limb_size;
        static constexpr hmpc::size limb_size = hmpc::core::limb_size_for<limb_type>(inner_limb_size * inner_limb_bit_size);

        inner_type inner;

        template<hmpc::size I>
        constexpr auto extended_read(hmpc::size_constant<I> index = {}) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type>);
            static_assert(index >= 0);
            if constexpr (index < limb_size)
            {
                constexpr hmpc::size begin_index = index * ratio;
                constexpr hmpc::size end_index = inner_limb_size;

                return hmpc::iter::scan_range<begin_index, end_index>([&](auto i, auto value)
                {
                    return hmpc::core::bit_or(
                        hmpc::core::cast<limb_type>(inner.extended_read(i)),
                        hmpc::core::shift_left(value, hmpc::size_constant_of<inner_limb_bit_size>)
                    );
                }, hmpc::zero_constant_of<limb_type>);
            }
            else
            {
                return hmpc::zero_constant_of<limb_type>;
            }
        }

        template<hmpc::size Index, hmpc::maybe_constant_of<limb_type> Value>
        constexpr void write(hmpc::size_constant<Index> index, Value value) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_write<access_type>);
            static_assert(index >= 0);
            static_assert(index < limb_size);

            constexpr hmpc::size begin_index = index * ratio;
            constexpr hmpc::size full_end_index = begin_index + ratio;

            constexpr hmpc::size end_index = [&]()
            {
                if constexpr (full_end_index < inner_limb_size)
                {
                    return full_end_index;
                }
                else
                {
                    return inner_limb_size;
                }
            }();

            hmpc::iter::scan_range<begin_index, end_index>([&](auto i, auto value)
            {
                inner.write(i, hmpc::core::cast<inner_limb_type>(value));

                return hmpc::core::shift_right(value, hmpc::size_constant_of<inner_limb_bit_size>);
            }, value);
        }
    };

    template<typename Limb, hmpc::limb_span Inner>
    constexpr auto convert_limb_span(Inner inner) noexcept
    {
        return converting_limb_span<Limb, Inner>{inner};
    }
}
