#pragma once

#include <hmpc/comp/crypto/lhe/key.hpp>
#include <hmpc/detail/unique_tag.hpp>
#include <hmpc/expr/expression.hpp>
#include <hmpc/expr/tensor.hpp>

namespace hmpc::expr::crypto::lhe
{
    template<hmpc::expression A, hmpc::expression B>
        requires (std::same_as<typename A::value_type, typename B::value_type> and std::same_as<typename A::shape_type, typename B::shape_type> and A::shape_type::rank == 0)
    struct key_expression
    {
        using a_type = A;
        using b_type = B;

        using value_type = a_type::value_type;

        using is_tuple = void;
        static constexpr hmpc::size arity = 2;

        a_type a;
        b_type b;

        constexpr auto get(hmpc::size_constant<0>) const
        {
            return a;
        }

        constexpr auto get(hmpc::size_constant<1>) const
        {
            return b;
        }

        static constexpr auto owned_from_parts(hmpc::typing::universal_reference_to_rvalue auto&& a, hmpc::typing::universal_reference_to_rvalue auto&& b)
        {
            return hmpc::comp::crypto::lhe::key{std::move(a), std::move(b)};
        }
    };

    template<auto Tag = []{}, typename T>
    constexpr auto key(hmpc::comp::crypto::lhe::key<T>& key)
    {
        return key_expression{
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::zero)>(key.a),
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::one)>(key.b)
        };
    }
}
