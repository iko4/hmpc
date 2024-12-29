#pragma once

#include <hmpc/expr/expression.hpp>

namespace hmpc::expr
{
    template<hmpc::expression E>
    struct abs_expression
    {
        using inner_type = E;
        using value_type = decltype(abs(std::declval<typename E::value_type>()));

        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = inner_type::shape_type;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr hmpc::size arity = 1;

        inner_type inner;

        constexpr abs_expression(inner_type const& inner) noexcept
            : inner(inner)
        {
        }

        constexpr inner_type const& get(hmpc::size_constant<0>) const noexcept
        {
            return inner;
        }

        static constexpr hmpc::access::once_tag access(hmpc::size_constant<0>) noexcept
        {
            return {};
        }

        constexpr auto shape() const noexcept
        {
            return inner.shape();
        }

        static constexpr element_type operator()(hmpc::state_with_arity<1> auto const& state, hmpc::index_for<element_shape_type> auto const& index, auto& capabilities) noexcept
        {
            return abs(inner_type::operator()(state.get(hmpc::constants::zero), index, capabilities));
        }
    };

    template<hmpc::expression E>
    constexpr auto abs(E e)
    {
        return abs_expression<E>{e};
    }
}
