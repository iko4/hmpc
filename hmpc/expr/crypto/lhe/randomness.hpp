#pragma once

#include <hmpc/comp/crypto/lhe/randomness.hpp>
#include <hmpc/detail/unique_tag.hpp>
#include <hmpc/expr/expression.hpp>
#include <hmpc/expr/number_theoretic_transform.hpp>
#include <hmpc/expr/random/binomial.hpp>
#include <hmpc/expr/random/binomial_from_number_generator.hpp>
#include <hmpc/expr/random/uniform.hpp>
#include <hmpc/expr/random/uniform_from_number_generator.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/ints/poly.hpp>

namespace hmpc::expr::crypto::lhe
{
    template<hmpc::expression U, hmpc::expression V, hmpc::expression W>
        requires (std::same_as<typename U::value_type, typename V::value_type> and std::same_as<typename U::value_type, typename W::value_type>)
    struct randomness_expression
    {
        using u_type = U;
        using v_type = V;
        using w_type = W;

        using value_type = u_type::value_type;

        using is_tuple = void;
        static constexpr hmpc::size arity = 3;

        u_type u;
        v_type v;
        w_type w;

        constexpr auto get(hmpc::size_constant<0>) const
        {
            return u;
        }

        constexpr auto get(hmpc::size_constant<1>) const
        {
            return v;
        }

        constexpr auto get(hmpc::size_constant<2>) const
        {
            return w;
        }

        template<hmpc::expression OtherU, hmpc::expression OtherV, hmpc::expression OtherW>
        static constexpr auto from_parts(OtherU u, OtherV v, OtherW w)
        {
            return randomness_expression<OtherU, OtherV, OtherW>{u, v, w};
        }

        static constexpr auto owned_from_parts(hmpc::typing::universal_reference_to_rvalue auto&& u, hmpc::typing::universal_reference_to_rvalue auto&& v, hmpc::typing::universal_reference_to_rvalue auto&& w)
        {
            return hmpc::comp::crypto::lhe::randomness{std::move(u), std::move(v), std::move(w)};
        }
    };

    template<auto Tag = []{}, typename T, hmpc::size... Dimensions>
    constexpr auto randomness(hmpc::comp::crypto::lhe::randomness<T, Dimensions...>& randomness)
    {
        return randomness_expression{
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::zero)>(randomness.u),
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::one)>(randomness.v),
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::two)>(randomness.w)
        };
    }

    template<auto Tag = []{}, typename T, hmpc::size... Dimensions>
    constexpr auto randomness(hmpc::comp::tensor<T, Dimensions...>& u, hmpc::comp::tensor<T, Dimensions...>& v, hmpc::comp::tensor<T, Dimensions...>& w)
    {
        return randomness_expression{
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::zero)>(u),
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::one)>(v),
            hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, hmpc::constants::two)>(w)
        };
    }

    template<typename T, auto Tag = []{}, hmpc::maybe_rational_size_constant VarianceU = decltype(hmpc::constants::half), hmpc::maybe_rational_size_constant VarianceV = decltype(hmpc::constants::ten), hmpc::maybe_rational_size_constant VarianceW = decltype(hmpc::constants::ten), typename RngU = hmpc::expr::random::use_number_generator<hmpc::detail::unique_tag(Tag, hmpc::constants::zero)>, typename RngV = hmpc::expr::random::use_number_generator<hmpc::detail::unique_tag(Tag, hmpc::constants::one)>, typename RngW = hmpc::expr::random::use_number_generator<hmpc::detail::unique_tag(Tag, hmpc::constants::two)>, hmpc::statistical_security StatisticalSecurity = default_statistical_security>
    constexpr auto randomness(auto const& shape, VarianceU variance_u = {}, VarianceV variance_v = {}, VarianceW variance_w = {}, RngU rng_u = {}, RngV rng_v = {}, RngW rng_w = {})
    {
        using coeff_type = hmpc::ints::traits::coefficient_type_t<T>;

        auto u = hmpc::expr::number_theoretic_transform(
            hmpc::expr::random::centered_binomial<coeff_type>(
                rng_u,
                shape,
                variance_u
            )
        );

        auto v = hmpc::expr::number_theoretic_transform(
            hmpc::expr::random::centered_binomial<coeff_type>(
                rng_v,
                shape,
                variance_v
            )
        );

        auto w = hmpc::expr::number_theoretic_transform(
            hmpc::expr::random::centered_binomial<coeff_type>(
                rng_w,
                shape,
                variance_w
            )
        );

        return randomness_expression{u, v, w};
    }

    template<typename T, auto Tag = []{}, auto Bound, hmpc::maybe_rational_size_constant VarianceU = decltype(hmpc::constants::half), hmpc::maybe_rational_size_constant VarianceW = decltype(hmpc::constants::ten), typename RngU = hmpc::expr::random::use_number_generator<hmpc::detail::unique_tag(Tag, hmpc::constants::zero)>, typename RngV = hmpc::expr::random::use_number_generator<hmpc::detail::unique_tag(Tag, hmpc::constants::one)>, typename RngW = hmpc::expr::random::use_number_generator<hmpc::detail::unique_tag(Tag, hmpc::constants::two)>, hmpc::statistical_security StatisticalSecurity = default_statistical_security>
    constexpr auto drowning_randomness(auto const& shape, hmpc::constant<decltype(Bound), Bound> bound = {}, VarianceU variance_u = {}, VarianceW variance_w = {}, RngU rng_u = {}, RngV rng_v = {}, RngW rng_w = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> security = {})
    {
        using coeff_type = hmpc::ints::traits::coefficient_type_t<T>;

        auto u = hmpc::expr::number_theoretic_transform(
            hmpc::expr::random::centered_binomial<coeff_type>(
                rng_u,
                shape,
                variance_u
            )
        );

        auto v = hmpc::expr::number_theoretic_transform(
            hmpc::expr::random::drown_signed_uniform<coeff_type>(
                rng_v,
                shape,
                bound,
                security
            )
        );

        auto w = hmpc::expr::number_theoretic_transform(
            hmpc::expr::random::centered_binomial<coeff_type>(
                rng_w,
                shape,
                variance_w
            )
        );

        return randomness_expression{u, v, w};
    }
}
