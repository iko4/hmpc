#pragma once

#include <hmpc/comp/vector.hpp>
#include <hmpc/expr/expression.hpp>

namespace hmpc::expr
{
    template<hmpc::size N, hmpc::expression E>
        requires hmpc::scalar<typename E::value_type>
    struct vector_expression
    {
        using inner_type = E;
        using element_type = inner_type::value_type;
        using value_type = hmpc::comp::vector<element_type, N>;
        using shape_type = inner_type::shape_type;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr hmpc::size arity = 1;

        inner_type inner;

        constexpr vector_expression(inner_type const& inner) noexcept
            : inner(inner)
        {
        }

        constexpr inner_type const& get(hmpc::size_constant<0>) const noexcept
        {
            return inner;
        }

        static constexpr auto access(hmpc::size_constant<0>) noexcept
        {
            if constexpr (N >= 1)
            {
                return hmpc::access::multiple;
            }
            else
            {
                return hmpc::access::once;
            }
        }

        constexpr auto shape() const HMPC_NOEXCEPT
        {
            return inner.shape();
        }

        static constexpr element_type operator()(hmpc::state_with_arity<1> auto const& state, hmpc::index_for<element_shape_type> auto const& index, auto& capabilities) HMPC_NOEXCEPT
        {
            return inner_type::operator()(state.get(hmpc::constants::zero), hmpc::unsqueeze(index, hmpc::constants::minus_one, hmpc::force), capabilities);
        }
    };

    template<hmpc::size N, hmpc::expression E>
    constexpr auto vectorize(E e, hmpc::size_constant<N> = {}) noexcept
    {
        return vector_expression<N, E>{e};
    }
}
