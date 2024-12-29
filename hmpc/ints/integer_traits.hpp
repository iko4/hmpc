#pragma once

#include <hmpc/core/uint.hpp>
#include <hmpc/ints/mod.hpp>
#include <hmpc/ints/uint.hpp>

namespace hmpc::ints
{
    template<typename T>
    struct integer_traits;

    template<typename Underlying>
    struct integer_traits<hmpc::core::uint<Underlying>>
    {
        using type = hmpc::core::uint<Underlying>;

        static constexpr hmpc::size bit_size = type::bit_size;

        static constexpr type zero = {};
        static constexpr type one = type{1};

        static constexpr type all_zeros = {};
        static constexpr type all_ones = compl all_zeros;

        static constexpr type min = zero;
        static constexpr type max = all_ones;
    };

    template<hmpc::size Bits, typename Limb, typename Normalization>
    struct integer_traits<hmpc::ints::uint<Bits, Limb, Normalization>>
    {
        using type = hmpc::ints::uint<Bits, Limb, Normalization>;
        using limb_type = type::limb_type;
        using normal_type = type::normal_type;

        static constexpr hmpc::size bit_size = type::bit_size;
        static constexpr hmpc::size limb_size = type::limb_size;

        static constexpr type zero = {};
        static constexpr type one = type{limb_type{1}};

        static constexpr type all_zeros = {};
        static constexpr type all_ones = compl all_zeros;

        static constexpr type min = zero;
        static constexpr type max = all_ones;
    };

    template<hmpc::size Bits, hmpc::signedness Signedness, typename Limb, typename Normalization>
    struct integer_traits<hmpc::ints::bigint<Bits, Signedness, Limb, Normalization>>
    {
        using type = hmpc::ints::bigint<Bits, Signedness, Limb, Normalization>;
        using limb_type = type::limb_type;
        using normal_type = type::normal_type;

        static constexpr hmpc::size bit_size = type::bit_size;
        static constexpr hmpc::size limb_size = type::limb_size;

        static constexpr type zero = {};
        static constexpr type one = type{limb_type{1}};
    };

    template<auto Modulus>
    struct integer_traits<hmpc::ints::mod<Modulus>>
    {
        using type = hmpc::ints::mod<Modulus>;
        using limb_type = type::limb_type;
        using normal_type = type::normal_type;

        static constexpr hmpc::size bit_size = type::bit_size;
        static constexpr hmpc::size limb_size = type::limb_size;

        using unsigned_type = hmpc::ints::ubigint<bit_size, limb_type, normal_type>;

        static constexpr type zero = {};
        static constexpr type one = hmpc::ints::num::bit_copy<type>(type::reduced_auxiliary_modulus_span);

        static constexpr unsigned_type min_unsigned = {};
        static constexpr unsigned_type max_unsigned = static_cast<unsigned_type>(abs(type::modulus - hmpc::ints::one<limb_type>));
    };
}
