#pragma once

#include <hmpc/shape.hpp>
#include <hmpc/expr/random/number_generator.hpp>
#include <hmpc/random/number_generator.hpp>
#include <hmpc/random/uniform.hpp>
#include <hmpc/value.hpp>

namespace hmpc::expr::random
{
    template<hmpc::value T, hmpc::random::engine Engine = hmpc::default_random_engine, hmpc::statistical_security StatisticalSecurity = default_statistical_security, typename Tag = decltype([]{}), hmpc::size... Dimensions>
    struct uniform_expression_from_number_generator
    {
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = hmpc::shape<Dimensions...>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        using engine_type = Engine;
        using generator_type = number_generator_expression<engine_type>;

        static constexpr hmpc::size arity = 0;

        shape_type HMPC_PRIVATE_MEMBER(shape);
        generator_type number_generator;

        constexpr uniform_expression_from_number_generator(generator_type const& number_generator, shape_type shape) HMPC_NOEXCEPT
            : HMPC_PRIVATE_MEMBER(shape)(shape)
            , number_generator(number_generator)
        {
        }

        constexpr auto state(auto&) const noexcept
        {
            return number_generator_storage{number_generator, hmpc::expr::element_shape(*this)};
        }

        constexpr shape_type const& shape() const noexcept
        {
            return HMPC_PRIVATE_MEMBER(shape);
        }

        static constexpr element_type operator()(number_generator_storage<engine_type, element_shape_type> const& number_generator, hmpc::index_for<element_shape_type> auto const& index, auto&) HMPC_DEVICE_NOEXCEPT
        {
            auto random = number_generator.generator(index);
            return hmpc::random::uniform<element_type>(random, hmpc::constant_of<StatisticalSecurity>);
        }
    };

    template<hmpc::value T, typename Engine = hmpc::default_random_engine, hmpc::statistical_security StatisticalSecurity = default_statistical_security, typename Tag = decltype([]{}), hmpc::size... Dimensions>
    constexpr auto uniform(number_generator_expression<Engine> const& number_generator, hmpc::shape<Dimensions...> shape, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> = {}) noexcept
    {
        return uniform_expression_from_number_generator<T, Engine, StatisticalSecurity, Tag, Dimensions...>{number_generator, shape};
    }

    template<hmpc::value T, auto Bound, hmpc::signedness Signedness, hmpc::random::engine Engine = hmpc::default_random_engine, hmpc::statistical_security StatisticalSecurity = default_statistical_security, typename Tag = decltype([]{}), hmpc::size... Dimensions>
    struct drown_uniform_expression_from_number_generator
    {
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = hmpc::shape<Dimensions...>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        using engine_type = Engine;
        using generator_type = number_generator_expression<engine_type>;

        static constexpr hmpc::size arity = 0;

        shape_type HMPC_PRIVATE_MEMBER(shape);
        generator_type number_generator;

        constexpr drown_uniform_expression_from_number_generator(generator_type const& number_generator, shape_type shape) HMPC_NOEXCEPT
            : HMPC_PRIVATE_MEMBER(shape)(shape)
            , number_generator(number_generator)
        {
        }

        constexpr auto state(auto&) const noexcept
        {
            return number_generator_storage{number_generator, hmpc::expr::element_shape(*this)};
        }

        constexpr shape_type const& shape() const noexcept
        {
            return HMPC_PRIVATE_MEMBER(shape);
        }

        static constexpr element_type operator()(number_generator_storage<engine_type, element_shape_type> const& number_generator, hmpc::index_for<element_shape_type> auto const& index, auto&) HMPC_DEVICE_NOEXCEPT
        {
            auto random = number_generator.generator(index);
            return static_cast<element_type>(hmpc::random::drown_uniform(random, hmpc::constant_of<Bound>, hmpc::constant_of<Signedness>, hmpc::constant_of<StatisticalSecurity>));
        }
    };

    template<hmpc::value T, auto Bound, hmpc::random::engine Engine = hmpc::default_random_engine, hmpc::statistical_security StatisticalSecurity = default_statistical_security, typename Tag = decltype([]{}), hmpc::size... Dimensions>
    constexpr auto drown_unsigned_uniform(number_generator_expression<Engine> const& number_generator, hmpc::shape<Dimensions...> shape, hmpc::constant<decltype(Bound), Bound> = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> = {}) noexcept
    {
        return drown_uniform_expression_from_number_generator<T, Bound, hmpc::without_sign, Engine, StatisticalSecurity, Tag, Dimensions...>{number_generator, shape};
    }

    template<hmpc::value T, auto Bound, hmpc::random::engine Engine = hmpc::default_random_engine, hmpc::statistical_security StatisticalSecurity = default_statistical_security, typename Tag = decltype([]{}), hmpc::size... Dimensions>
    constexpr auto drown_signed_uniform(number_generator_expression<Engine> const& number_generator, hmpc::shape<Dimensions...> shape, hmpc::constant<decltype(Bound), Bound> = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> = {}) noexcept
    {
        return drown_uniform_expression_from_number_generator<T, Bound, hmpc::with_sign, Engine, StatisticalSecurity, Tag, Dimensions...>{number_generator, shape};
    }
}
