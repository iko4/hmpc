#pragma once

#include <hmpc/constant.hpp>
#include <hmpc/ints/bigint.hpp>
#include <hmpc/ints/num/bit_copy.hpp>
#include <hmpc/ints/numeric.hpp>
#include <hmpc/iter/scan_range.hpp>
#include <hmpc/iter/scan_reverse_range.hpp>

#include <array>
#include <span>

namespace hmpc::detail::strings
{
    template<typename Char, hmpc::size Size>
    struct basic_fixed_string
    {
        using char_type = Char;

        static constexpr auto size = hmpc::size_constant_of<Size>;

        std::array<char_type, size> data;

        template<typename... Chars>
        constexpr basic_fixed_string(Chars... chars) noexcept
            : data{chars...}
        {
            static_assert(sizeof...(Chars) == size);
        }

        template<std::convertible_to<char_type const> T>
        constexpr basic_fixed_string(std::span<T, size> string) noexcept
        {
            std::copy_n(begin(string), size, begin(data));
        }

        constexpr char_type operator[](hmpc::size i) const
        {
            return data[i];
        }

        template<hmpc::size Offset, hmpc::size SubstringLength = size - Offset>
        constexpr auto substr(hmpc::size_constant<Offset> offset = {}, hmpc::size_constant<SubstringLength> substring_length = {}) const noexcept
        {
            static_assert(offset >= 0);
            static_assert(offset < size);
            static_assert(substring_length >= 0);
            static_assert(offset + substring_length <= size);

            std::span<char_type const, size> string = data;

            return basic_fixed_string<char_type, substring_length>(string.template subspan<offset, substring_length>());
        }
    };

    template<typename Char, typename... Chars>
    basic_fixed_string(Char, Chars...) -> basic_fixed_string<Char, sizeof...(Chars) + 1>;
}

namespace hmpc::ints::literals
{
    template<hmpc::detail::strings::basic_fixed_string String>
    consteval auto parse_binary_literal()
    {
        constexpr hmpc::size bit_size = hmpc::iter::scan(hmpc::range(String.size), [](auto i, auto size)
        {
            switch (String[i])
            {
            case '0':
                return (size == hmpc::size{}) ? size : size + 1;
            case '1':
                return size + 1;
            case '\'':
                return size;
            default:
                HMPC_COMPILETIME_ASSERT(false);
                return size;
            }
        }, hmpc::size{});

        hmpc::ints::ubigint<bit_size> result = {};
        using limb_type = decltype(result)::limb_type;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        hmpc::iter::scan_reverse(hmpc::range(String.size), [result = result.span(hmpc::access::read_write)](auto i, auto j)
        {
            constexpr auto c = String[i];
            if constexpr (c == '0')
            {
                return hmpc::size_constant_of<j + 1>;
            }
            else if constexpr (c == '1')
            {
                constexpr auto bit = hmpc::size_constant_of<j % result.limb_bit_size>;
                constexpr auto limb = hmpc::size_constant_of<j / result.limb_bit_size>;
                result[limb] |= limb_traits::one << bit;
                return hmpc::size_constant_of<j + 1>;
            }
            else
            {
                return j;
            }
        }, hmpc::constants::zero);
        return result;
    }

    template<hmpc::detail::strings::basic_fixed_string String>
    consteval auto parse_hex_literal()
    {
        constexpr hmpc::size bit_size = hmpc::iter::scan(hmpc::range(String.size), [](auto i, auto size)
        {
            switch (String[i])
            {
            case '0':
                return (size == hmpc::size{}) ? size : size + 4;
            case '1':
                return (size == hmpc::size{}) ? size + 1 : size + 4;
            case '2':
            case '3':
                return (size == hmpc::size{}) ? size + 2 : size + 4;
            case '4':
            case '5':
            case '6':
            case '7':
                return (size == hmpc::size{}) ? size + 3 : size + 4;
            case '8':
            case '9':
            case 'a':
            case 'A':
            case 'b':
            case 'B':
            case 'c':
            case 'C':
            case 'd':
            case 'D':
            case 'e':
            case 'E':
            case 'f':
            case 'F':
                return size + 4;
            case '\'':
                return size;
            default:
                HMPC_COMPILETIME_ASSERT(false);
                return size;
            }
        }, hmpc::size{});

        hmpc::ints::ubigint<bit_size> result = {};
        using limb_type = decltype(result)::limb_type;
        static_assert(result.limb_bit_size >= 4);
        static_assert(result.limb_bit_size % 4 == 0);
        hmpc::iter::scan_reverse(hmpc::range(String.size), [result = result.span(hmpc::access::read_write)](auto i, auto j)
        {
            constexpr auto bit = hmpc::size_constant_of<j % result.limb_bit_size>;
            constexpr auto limb = hmpc::size_constant_of<j / result.limb_bit_size>;
            constexpr auto c = String[i];
            if constexpr ('0' <= c and c <= '9')
            {
                result[limb] |= limb_type{c - '0'} << bit;
                return hmpc::size_constant_of<j + 4>;
            }
            else if constexpr ('a' <= c and c <= 'f')
            {
                result[limb] |= limb_type{c - 'a' + 10} << bit;
                return hmpc::size_constant_of<j + 4>;
            }
            else if constexpr ('A' <= c and c <= 'F')
            {
                result[limb] |= limb_type{c - 'A' + 10} << bit;
                return hmpc::size_constant_of<j + 4>;
            }
            else
            {
                return j;
            }
        }, hmpc::constants::zero);
        return result;
    }

    template<hmpc::detail::strings::basic_fixed_string String>
    consteval auto parse_decimal_literal()
    {
        constexpr auto large_result = hmpc::iter::scan(hmpc::range(String.size), [](auto i, auto result)
        {
            constexpr auto ten = hmpc::ints::ubigint<4>{10};
            constexpr auto c = String[i];
            if constexpr (c == '0')
            {
                return result * ten;
            }
            else if constexpr ('1' <= c and c <= '9')
            {
                constexpr auto bits = std::bit_width(static_cast<std::size_t>(c - '0'));
                return result * ten + hmpc::ints::ubigint<bits>{c - '0'};
            }
            else if constexpr (c == '\'')
            {
                return result;
            }
        }, hmpc::ints::ubigint<0>{});
        constexpr auto bits = bit_width(large_result);
        hmpc::ints::ubigint<bits> result;
        hmpc::ints::num::bit_copy(result, large_result);
        return result;
    }

    template<hmpc::detail::strings::basic_fixed_string String>
    consteval auto parse_literal()
    {
        auto two = hmpc::size_constant_of<2>;
        if constexpr (String.size >= 2 and String[0] == '0' and (String[1] == 'b' or String[1] == 'B'))
        {
            return parse_binary_literal<String.substr(two)>();
        }
        else if constexpr (String.size >= 2 and String[0] == '0' and (String[1] == 'x' or String[1] == 'X'))
        {
            return parse_hex_literal<String.substr(two)>();
        }
        else
        {
            return parse_decimal_literal<String>();
        }
    }

    template<char... Chars>
    consteval auto operator""_int()
    {
        constexpr hmpc::detail::strings::basic_fixed_string string = {Chars...};
        return parse_literal<string>();
    }
}
