#pragma once

#include <hmpc/comp/crypto/lhe/ciphertext.hpp>
#include <hmpc/detail/unique_tag.hpp>
#include <hmpc/expr/expression.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/typing/reference.hpp>
#include <hmpc/expr/binary_expression.hpp>

namespace hmpc::expr::crypto::lhe
{
    template<hmpc::expression C0, hmpc::expression C1>
        requires std::same_as<typename C0::value_type, typename C1::value_type>
    struct ciphertext_expression
    {
        using c0_type = C0;
        using c1_type = C1;

        using value_type = c0_type::value_type;

        using is_tuple = void;
        static constexpr hmpc::size arity = 2;

        c0_type c0;
        c1_type c1;

        constexpr auto get(hmpc::size_constant<0>) const
        {
            return c0;
        }

        constexpr auto get(hmpc::size_constant<1>) const
        {
            return c1;
        }

        template<hmpc::expression OtherC0, hmpc::expression OtherC1>
        static constexpr auto from_parts(OtherC0 c0, OtherC1 c1)
        {
            return ciphertext_expression<OtherC0, OtherC1>{c0, c1};
        }

        static constexpr auto owned_from_parts(hmpc::typing::universal_reference_to_rvalue auto&& c0, hmpc::typing::universal_reference_to_rvalue auto&& c1)
        {
            return hmpc::comp::crypto::lhe::ciphertext{std::move(c0), std::move(c1)};
        }

        template<hmpc::expression OtherC0, hmpc::expression OtherC1>
        friend constexpr auto operator+(ciphertext_expression left, ciphertext_expression<OtherC0, OtherC1> right)
        {
            using namespace hmpc::expr::operators;
            return ciphertext_expression<decltype(left.c0 + right.c0), decltype(left.c1 + right.c1)>{left.c0 + right.c0, left.c1 + right.c1};
        }

        template<hmpc::expression OtherC0, hmpc::expression OtherC1>
        friend constexpr auto operator-(ciphertext_expression left, ciphertext_expression<OtherC0, OtherC1> right)
        {
            using namespace hmpc::expr::operators;
            return ciphertext_expression<decltype(left.c0 - right.c0), decltype(left.c1 - right.c1)>{left.c0 - right.c0, left.c1 - right.c1};
        }

        template<hmpc::expression OtherC0, hmpc::expression OtherC1>
        friend constexpr auto operator==(ciphertext_expression left, ciphertext_expression<OtherC0, OtherC1> right)
        {
            using namespace hmpc::expr::operators;
            return (left.c0 == right.c0) bitand (left.c1 == right.c1);
        }

        template<hmpc::expression Right>
        friend constexpr auto operator*(ciphertext_expression left, Right right)
        {
            using namespace hmpc::expr::operators;
            return ciphertext_expression<decltype(left.c0 * right), decltype(left.c1 * right)>{left.c0 * right, left.c1 * right};
        }

        template<hmpc::expression Left>
        friend constexpr auto operator*(Left left, ciphertext_expression right)
        {
            using namespace hmpc::expr::operators;
            return ciphertext_expression<decltype(left * right.c0), decltype(left * right.c1)>{left * right.c0, left * right.c1};
        }
    };

    template<auto Tag = []{}, typename T, hmpc::size... Dimensions>
    constexpr auto ciphertext(hmpc::comp::crypto::lhe::ciphertext<T, Dimensions...>& ciphertext)
    {
        return ciphertext_expression{
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::zero)>(ciphertext.c0),
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::one)>(ciphertext.c1)
        };
    }

    template<auto Tag = []{}, typename T, hmpc::size... Dimensions>
    constexpr auto ciphertext(hmpc::comp::tensor<T, Dimensions...>& c0, hmpc::comp::tensor<T, Dimensions...>& c1)
    {
        return ciphertext_expression{
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::zero)>(c0),
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::one)>(c1)
        };
    }
}
