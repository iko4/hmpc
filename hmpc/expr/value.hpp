#pragma once

#include <hmpc/config.hpp>
#include <hmpc/value.hpp>

namespace hmpc::expr
{
    template<hmpc::scalar T>
    struct base_value_expression
    {
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;

        using shape_type = hmpc::shape<>;
        using element_shape_type = shape_type;

        static constexpr hmpc::size arity = 0;

        constexpr shape_type shape() const noexcept
        {
            return {};
        }

        static constexpr auto const& operator()(value_type const& value, hmpc::index_for<element_shape_type> auto const&, auto const&) noexcept
        {
            return value;
        }
    };

    template<hmpc::scalar T, typename Tag = decltype([]{})>
    struct value_expression : public base_value_expression<T>
    {
        using typename base_value_expression<T>::value_type;

        value_type value;

        constexpr value_expression(value_type&& value) noexcept
            : value(std::move(value))
        {
        }

        constexpr auto state(auto&) const noexcept
        {
            return value;
        }
    };

    template<hmpc::scalar T, typename Tag = decltype([]{})>
    struct value_view_expression : public base_value_expression<T>
    {
        using typename base_value_expression<T>::value_type;

        value_type const* value;

        constexpr value_view_expression(value_type const& value) noexcept
            : value(std::addressof(value))
        {
        }

        constexpr auto state(auto&) const noexcept
        {
            return *value;
        }
    };

    template<hmpc::scalar T, typename Tag = decltype([]{})>
    constexpr auto value(T const& v) noexcept
    {
        return value_view_expression<T, Tag>{v};
    }

    template<hmpc::scalar T, typename Tag = decltype([]{})>
    constexpr auto value(T&& v) noexcept
    {
        return value_expression<T, Tag>{std::move(v)};
    }
}
