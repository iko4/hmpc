#pragma once

#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/cast.hpp>
#include <hmpc/expr/crypto/lhe/ciphertext.hpp>
#include <hmpc/expr/crypto/lhe/key.hpp>
#include <hmpc/expr/number_theoretic_transform.hpp>
#include <hmpc/expr/unsqueeze.hpp>
#include <hmpc/ints/poly.hpp>

namespace hmpc::expr::crypto::lhe
{
    template<typename Plaintext, hmpc::expression Key, hmpc::expression C0, hmpc::expression C1>
    constexpr auto dec(Key key, hmpc::expr::crypto::lhe::ciphertext_expression<C0, C1> ciphertext)
    {
        using namespace hmpc::expr::operators;

        using poly_type = Key::value_type;
        static_assert(poly_type::representation == hmpc::ints::number_theoretic_transform_representation);

        constexpr auto rank = C1::shape_type::rank;
        static_assert(Key::shape_type::rank == 0);

        auto s = hmpc::iter::scan_range<rank>([](auto, auto s)
        {
            return hmpc::expr::unsqueeze(s, hmpc::constants::minus_one);
        }, key);

        auto x = ciphertext.c0 - s * ciphertext.c1;

        auto coeff_x = hmpc::expr::inverse_number_theoretic_transform(x);

        if constexpr (Plaintext::representation == hmpc::ints::number_theoretic_transform_representation)
        {
            using coeff_type = hmpc::ints::traits::coefficient_type_t<Plaintext>;
            return hmpc::expr::number_theoretic_transform(
                hmpc::expr::cast<coeff_type>(coeff_x)
            );
        }
        else
        {
            static_assert(Plaintext::representation == hmpc::ints::coefficient_representation);
            return hmpc::expr::cast<Plaintext>(coeff_x);
        }
    }
}
