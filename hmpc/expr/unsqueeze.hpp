#pragma once

#include <hmpc/comp/accessor.hpp>
#include <hmpc/expr/expression.hpp>

namespace hmpc::expr
{
    template<hmpc::signed_size Dim, hmpc::expression E>
    struct unsqueeze_expression
    {
        using inner_type = E;
        using value_type = inner_type::value_type;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = decltype(hmpc::unsqueeze(std::declval<typename inner_type::shape_type>(), hmpc::signed_size_constant_of<Dim>));
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr auto arity = hmpc::size_constant_of<1>;

        inner_type inner;

        constexpr unsqueeze_expression(inner_type const& inner, hmpc::signed_size_constant<Dim> = {}) HMPC_NOEXCEPT
            : inner(inner)
        {
        }

        constexpr inner_type const& get(hmpc::size_constant<0>) const HMPC_NOEXCEPT
        {
            return inner;
        }

        static constexpr hmpc::access::multiple_tag access(hmpc::size_constant<0>) noexcept
        {
            return {};
        }

        constexpr auto shape() const HMPC_NOEXCEPT
        {
            return hmpc::unsqueeze(inner.shape(), hmpc::signed_size_constant_of<Dim>);
        }

        static constexpr element_type operator()(hmpc::state_with_arity<1> auto const& state, hmpc::mdindex_for<element_shape_type> auto const& index, auto& capabilities) HMPC_NOEXCEPT
        {
            return inner_type::operator()(state.get(hmpc::constants::zero), hmpc::unsqueeze_for_element_index<value_type>(index, hmpc::signed_size_constant_of<Dim>, hmpc::force), capabilities);
        }
    };

    template<hmpc::signed_size Dim, hmpc::expression E>
    constexpr auto unsqueeze(E const& e, hmpc::signed_size_constant<Dim> dim = {}) HMPC_NOEXCEPT
    {
        return unsqueeze_expression{e, dim};
    }

    template<hmpc::signed_size Dim, hmpc::expression_tuple E>
    constexpr auto unsqueeze(E e, hmpc::signed_size_constant<Dim> dim = {}) HMPC_NOEXCEPT
    {
        return hmpc::iter::unpack(hmpc::range(E::arity), [&](auto... i)
        {
            return E::from_parts(unsqueeze(e.get(i), dim)...);
        });
    }
}
