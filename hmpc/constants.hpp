#pragma once

#include <hmpc/constant.hpp>
#include <hmpc/core/uint.hpp>
#include <hmpc/signedness.hpp>

namespace hmpc::constants
{
    constexpr hmpc::false_type no = {};
    constexpr hmpc::true_type yes = {};

    constexpr auto without_sign = hmpc::constant_of<hmpc::without_sign>;
    constexpr auto with_sign = hmpc::constant_of<hmpc::with_sign>;

    constexpr auto zero = hmpc::zero_constant_of<hmpc::size>;
    constexpr auto one = hmpc::size_constant_of<1>;
    constexpr auto two = hmpc::size_constant_of<2>;
    constexpr auto three = hmpc::size_constant_of<3>;
    constexpr auto four = hmpc::size_constant_of<4>;
    constexpr auto five = hmpc::size_constant_of<5>;
    constexpr auto six = hmpc::size_constant_of<6>;
    constexpr auto seven = hmpc::size_constant_of<7>;
    constexpr auto eight = hmpc::size_constant_of<8>;
    constexpr auto nine = hmpc::size_constant_of<9>;
    constexpr auto ten = hmpc::size_constant_of<10>;
    constexpr auto minus_one = hmpc::signed_size_constant_of<-1>;
    constexpr auto minus_two = hmpc::signed_size_constant_of<-2>;
    constexpr auto placeholder = hmpc::size_constant_of<hmpc::placeholder_extent>;

    namespace bit
    {
        constexpr auto zero = hmpc::bit_constant_of<hmpc::bit{false}>;
        constexpr auto one = hmpc::bit_constant_of<hmpc::bit{true}>;
    }

    namespace security
    {
        constexpr auto statistical = hmpc::constant_of<hmpc::default_statistical_security>;
        constexpr auto computational = hmpc::constant_of<hmpc::default_computational_security>;
    }
}
