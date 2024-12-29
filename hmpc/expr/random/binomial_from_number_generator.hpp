#pragma once

#include <hmpc/shape.hpp>
#include <hmpc/expr/random/number_generator.hpp>
#include <hmpc/random/binomial.hpp>
#include <hmpc/random/number_generator.hpp>
#include <hmpc/value.hpp>

namespace hmpc::expr::random
{
    template<hmpc::value T, hmpc::rational_size Variance, typename Engine = hmpc::default_random_engine, typename Tag = decltype([]{}), hmpc::size... Dimensions>
    struct centered_binomial_expression_from_number_generator
    {
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = hmpc::shape<Dimensions...>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        using engine_type = Engine;
        using number_generator_expression_type = number_generator_expression<engine_type>;

        static constexpr hmpc::size arity = 0;

        shape_type HMPC_PRIVATE_MEMBER(shape);
        number_generator_expression_type number_generator;

        constexpr centered_binomial_expression_from_number_generator(number_generator_expression_type const& number_generator, shape_type const& shape) HMPC_NOEXCEPT
            : HMPC_PRIVATE_MEMBER(shape)(shape)
            , number_generator(number_generator)
        {
        }

        constexpr shape_type const& shape() const noexcept
        {
            return HMPC_PRIVATE_MEMBER(shape);
        }

        constexpr auto state(auto&) const noexcept
        {
            return number_generator_storage{number_generator, hmpc::expr::element_shape(*this)};
        }

        static constexpr element_type operator()(number_generator_storage<engine_type, element_shape_type> const& number_generator, hmpc::index_for<element_shape_type> auto const& index, auto&) HMPC_DEVICE_NOEXCEPT
        {
            auto random = number_generator.generator(index);
            return static_cast<element_type>(hmpc::random::centered_binomial(random, hmpc::constant_of<Variance>));
        }
    };

    template<hmpc::value T, hmpc::size Variance, typename Engine = hmpc::default_random_engine, typename Tag = decltype([]{}), hmpc::size... Dimensions>
    constexpr auto centered_binomial(number_generator_expression<Engine> const& number_generator, hmpc::shape<Dimensions...> shape, hmpc::size_constant<Variance> = {}) noexcept
    {
        return centered_binomial_expression_from_number_generator<T, Variance, Engine, Tag, Dimensions...>{number_generator, shape};
    }

    template<hmpc::value T, hmpc::rational_size Variance, typename Engine = hmpc::default_random_engine, typename Tag = decltype([]{}), hmpc::size... Dimensions>
    constexpr auto centered_binomial(number_generator_expression<Engine> const& number_generator, hmpc::shape<Dimensions...> shape, hmpc::rational_size_constant<Variance> = {}) noexcept
    {
        return centered_binomial_expression_from_number_generator<T, Variance, Engine, Tag, Dimensions...>{number_generator, shape};
    }
}
