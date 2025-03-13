#pragma once

#include <hmpc/value.hpp>

namespace hmpc::expr
{
    template<hmpc::scalar auto Value>
    struct constant_expression
    {
        using value_type = decltype(Value);
        using element_type = hmpc::traits::element_type_t<value_type>;

        using shape_type = hmpc::shape<>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr auto arity = hmpc::size_constant_of<0>;

        constexpr shape_type shape() const noexcept
        {
            return {};
        }

        constexpr auto state(auto&) const noexcept
        {
            return hmpc::constant_of<Value>;
        }

        static constexpr auto operator()(hmpc::constant<value_type, Value> value, hmpc::index_for<element_shape_type> auto const&, auto const&) noexcept
        {
            return value.value;
        }
    };

    template<hmpc::scalar auto Value>
    constexpr constant_expression<Value> constant_of = {};

    template<hmpc::scalar T, T V>
    constexpr constant_expression<V> constant(hmpc::constant<T, V>) noexcept
    {
        return {};
    }
}
