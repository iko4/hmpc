#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/shift_left.hpp>
#include <hmpc/core/shift_right.hpp>
#include <hmpc/iter/next.hpp>
#include <hmpc/iter/stride.hpp>

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

        static constexpr auto limb_bit_size = limb_type::bit_size;
        static constexpr auto inner_limb_bit_size = inner_type::limb_bit_size;

        static_assert(limb_bit_size == inner_limb_bit_size);

        static constexpr auto inner_limb_size = inner_type::limb_size;
        static constexpr auto limb_size = inner_limb_size;

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

        static constexpr auto limb_bit_size = limb_type::bit_size;
        static constexpr auto inner_limb_bit_size = inner_type::limb_bit_size;

        static_assert(limb_bit_size > inner_limb_bit_size);
        static_assert(limb_bit_size % inner_limb_bit_size == 0);

        static constexpr auto ratio = hmpc::size_constant_of<limb_bit_size / inner_limb_bit_size>;

        static constexpr auto inner_limb_size = inner_type::limb_size;
        static constexpr auto limb_size = hmpc::core::limb_size_for<limb_type>(hmpc::iter::stride(inner_limb_size, inner_limb_bit_size));

        inner_type inner;

        template<hmpc::size I>
        constexpr auto extended_read(hmpc::size_constant<I> index = {}) const HMPC_NOEXCEPT
        {
            static_assert(hmpc::access::is_read<access_type>);
            static_assert(index >= 0);
            if constexpr (index < limb_size)
            {
                constexpr auto begin_index = hmpc::iter::stride(index, ratio);
                constexpr auto end_index = inner_limb_size;

                return hmpc::iter::scan(hmpc::range(begin_index, end_index), [&](auto i, auto value)
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

            constexpr auto begin_index = hmpc::iter::stride(index, ratio);
            constexpr auto full_end_index = hmpc::iter::next(begin_index, ratio);

            constexpr auto end_index = [&]()
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

            hmpc::iter::scan(hmpc::range(begin_index, end_index), [&](auto i, auto value)
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
