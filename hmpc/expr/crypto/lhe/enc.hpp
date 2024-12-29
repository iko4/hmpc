#pragma once

#include <hmpc/expr/crypto/lhe/ciphertext.hpp>
#include <hmpc/expr/crypto/lhe/key.hpp>
#include <hmpc/expr/crypto/lhe/randomness.hpp>
#include <hmpc/constant.hpp>
#include <hmpc/typing/reference.hpp>
#include <hmpc/detail/unique_tag.hpp>
#include <hmpc/expr/expression.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/cast.hpp>
#include <hmpc/expr/constant.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/expr/number_theoretic_transform.hpp>
#include <hmpc/expr/unsqueeze.hpp>

namespace hmpc::expr::crypto::lhe
{
    template<hmpc::expression A, hmpc::expression B, hmpc::expression Plaintext, hmpc::expression U, hmpc::expression V, hmpc::expression W>
    constexpr auto enc(hmpc::expr::crypto::lhe::key_expression<A, B> key, Plaintext plaintext, hmpc::expr::crypto::lhe::randomness_expression<U, V, W> randomness)
    {
        using poly_type = A::value_type;
        static_assert(poly_type::representation == hmpc::ints::number_theoretic_transform_representation);
        using coeff_type = hmpc::ints::traits::coefficient_type_t<poly_type>;

        auto shape = plaintext.shape();
        constexpr auto rank = decltype(shape)::rank;

        auto a = hmpc::iter::scan_range<rank>([](auto, auto a)
        {
            return hmpc::expr::unsqueeze(a, hmpc::constants::minus_one);
        }, key.get(hmpc::constants::zero));

        auto b = hmpc::iter::scan_range<rank>([](auto, auto b)
        {
            return hmpc::expr::unsqueeze(b, hmpc::constants::minus_one);
        }, key.get(hmpc::constants::one));

        using namespace hmpc::expr::operators;

        using plaintext_type = Plaintext::value_type;

        auto message = hmpc::expr::number_theoretic_transform(
            hmpc::expr::cast<coeff_type>(
                [&]()
                {
                    if constexpr (plaintext_type::representation == hmpc::ints::number_theoretic_transform_representation)
                    {
                        return hmpc::expr::inverse_number_theoretic_transform(plaintext);
                    }
                    else
                    {
                        return plaintext;
                    }
                }()
            )
        );

        constexpr auto p = hmpc::expr::constant(
            hmpc::constant_cast<typename poly_type::element_type>(
                plaintext_type::element_type::modulus_constant
            )
        );

        return ciphertext_expression{
            a * randomness.u + randomness.v * p + message,
            b * randomness.u + randomness.w * p
        };
    }
}
