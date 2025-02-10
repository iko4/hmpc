#pragma once

#include <hmpc/detail/tuple.hpp>
#include <hmpc/shape.hpp>
#include <hmpc/value.hpp>

namespace hmpc
{
    template<typename E>
    concept expression = requires(E expr)
    {
        typename std::remove_cvref_t<E>::value_type;
        typename std::remove_cvref_t<E>::shape_type;
        std::remove_cvref_t<E>::arity;
        expr.shape();
    } and hmpc::value<typename std::remove_cvref_t<E>::value_type>;

    template<typename E>
    concept expression_with_capabilities = expression<E> and requires
    {
        typename std::remove_cvref_t<E>::capabilities;
        std::remove_cvref_t<E>::capabilities::size;
    };

    template<typename E>
    concept complex_expression = expression<E> and requires
    {
        typename std::remove_cvref_t<E>::is_complex;
    };

    template<typename E>
    concept expression_tuple = requires
    {
        std::remove_cvref_t<E>::arity;
        typename std::remove_cvref_t<E>::is_tuple;
    };

    template<typename State, hmpc::size Arity>
    concept state_with_arity = (State::arity == Arity);
}

namespace hmpc::expr
{
    namespace capabilities
    {
        struct random_number_generator_tag
        {
        };
        constexpr random_number_generator_tag random_number_generator = {};
    }

    namespace traits
    {
        template<typename E>
        using element_shape_t = hmpc::traits::element_shape_t<typename E::value_type, typename E::shape_type>;

        template<typename E>
        using element_type_t = hmpc::traits::element_type_t<typename E::value_type>;
    }

    namespace detail
    {
        template<typename Shape, hmpc::expression E>
        struct same_element_shape : std::bool_constant<hmpc::iter::for_packed_range<E::arity>([](auto... i)
        {
            if constexpr (E::arity > 0)
            {
                return (same_element_shape<Shape, std::remove_cvref_t<decltype(std::declval<E>().get(i))>>::value and ... and std::same_as<Shape, hmpc::expr::traits::element_shape_t<E>>);
            }
            else
            {
                return std::same_as<Shape, hmpc::expr::traits::element_shape_t<E>>;
            }
        })>
        {
        };
    }

    template<typename Expression, typename Shape = traits::element_shape_t<Expression>>
    concept same_element_shape = detail::same_element_shape<Shape, Expression>::value;

    template<typename... States>
    struct multi_state : public hmpc::detail::tuple<States...>
    {
        static constexpr hmpc::size arity = sizeof...(States);
    };

    template<typename... States>
    constexpr auto make_state(States... states) HMPC_NOEXCEPT
    {
        return multi_state<States...>{states...};
    }

    template<hmpc::expression E>
    constexpr auto element_shape(E&& expr) HMPC_NOEXCEPT
    {
        using value_type = std::remove_cvref_t<E>::value_type;
        return hmpc::element_shape<value_type>(std::forward<E>(expr).shape());
    }
}
