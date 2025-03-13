#pragma once

#include <hmpc/comp/accessor.hpp>
#include <hmpc/expr/expression.hpp>

namespace hmpc::expr
{
    template<hmpc::value T, hmpc::expression E>
        requires (hmpc::traits::vector_size_v<T> == hmpc::traits::vector_size_v<typename E::value_type>)
    struct cast_expression
    {
        using inner_type = E;
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = inner_type::shape_type;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr auto arity = hmpc::size_constant_of<1>;

        inner_type inner;

        constexpr cast_expression(inner_type const& inner) HMPC_NOEXCEPT
            : inner(inner)
        {
        }

        constexpr inner_type const& get(hmpc::size_constant<0>) const HMPC_NOEXCEPT
        {
            return inner;
        }

        static constexpr hmpc::access::once_tag access(hmpc::size_constant<0>) noexcept
        {
            return {};
        }

        constexpr auto shape() const HMPC_NOEXCEPT
        {
            return inner.shape();
        }

        static constexpr element_type operator()(hmpc::state_with_arity<1> auto const& state, hmpc::index_for<element_shape_type> auto const& index, auto& capabilities) HMPC_NOEXCEPT
        {
            return static_cast<element_type>(inner_type::operator()(state.get(hmpc::constants::zero), index, capabilities));
        }
    };

    template<hmpc::value T, hmpc::expression E>
    constexpr auto cast(E const& e) HMPC_NOEXCEPT
    {
        return cast_expression<T, E>{e};
    }
}
