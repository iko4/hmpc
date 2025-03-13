#pragma once

#include <hmpc/expr/random/number_generator.hpp>
#include <hmpc/random/uniform.hpp>
#include <hmpc/shape.hpp>
#include <hmpc/value.hpp>

namespace hmpc::expr::random
{
    template<hmpc::value T, auto Tag = []{}, hmpc::statistical_security StatisticalSecurity = default_statistical_security, hmpc::size... Dimensions>
    struct uniform_expression : public enable_caching
    {
        using enable_caching::operator();

        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = hmpc::shape<Dimensions...>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr auto arity = hmpc::size_constant_of<0>;

        using capabilities = hmpc::detail::type_list<hmpc::expr::capabilities::random_number_generator_tag>;

        shape_type HMPC_PRIVATE_MEMBER(shape);

        constexpr uniform_expression(shape_type shape) HMPC_NOEXCEPT
            : HMPC_PRIVATE_MEMBER(shape)(shape)
        {
        }

        constexpr hmpc::monostate state(auto&) const noexcept
        {
            return hmpc::empty;
        }

        constexpr shape_type const& shape() const noexcept
        {
            return HMPC_PRIVATE_MEMBER(shape);
        }

        static constexpr element_type operator()(hmpc::monostate, hmpc::index_for<element_shape_type> auto const&, auto& capabilities) HMPC_DEVICE_NOEXCEPT
        {
            auto& random = capabilities.get(hmpc::expr::capabilities::random_number_generator);
            return hmpc::random::uniform<element_type>(random, hmpc::constant_of<StatisticalSecurity>);
        }
    };

    template<hmpc::value T, auto Tag = []{}, hmpc::statistical_security StatisticalSecurity = default_statistical_security, hmpc::size... Dimensions>
    constexpr auto uniform(hmpc::shape<Dimensions...> shape, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> = {}) noexcept
    {
        return uniform_expression<T, Tag, StatisticalSecurity, Dimensions...>{shape};
    }

    template<hmpc::value T, auto Tag = []{}, hmpc::statistical_security StatisticalSecurity = default_statistical_security, hmpc::size... Dimensions>
    constexpr auto uniform(hmpc::expr::random::use_number_generator<Tag>, hmpc::shape<Dimensions...> shape, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> = {}) noexcept
    {
        return uniform_expression<T, Tag, StatisticalSecurity, Dimensions...>{shape};
    }

    template<hmpc::value T, auto Bound, hmpc::signedness Signedness, auto Tag = []{}, hmpc::statistical_security StatisticalSecurity = default_statistical_security, hmpc::size... Dimensions>
    struct drown_uniform_expression : public enable_caching
    {
        using enable_caching::operator();

        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = hmpc::shape<Dimensions...>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr auto arity = hmpc::size_constant_of<0>;

        using capabilities = hmpc::detail::type_list<hmpc::expr::capabilities::random_number_generator_tag>;

        shape_type HMPC_PRIVATE_MEMBER(shape);

        constexpr drown_uniform_expression(shape_type shape) HMPC_NOEXCEPT
            : HMPC_PRIVATE_MEMBER(shape)(shape)
        {
        }

        constexpr hmpc::monostate state(auto&) const noexcept
        {
            return hmpc::empty;
        }

        constexpr shape_type const& shape() const noexcept
        {
            return HMPC_PRIVATE_MEMBER(shape);
        }

        static constexpr element_type operator()(hmpc::monostate, hmpc::index_for<element_shape_type> auto const&, auto& capabilities) HMPC_DEVICE_NOEXCEPT
        {
            auto& random = capabilities.get(hmpc::expr::capabilities::random_number_generator);
            return static_cast<element_type>(hmpc::random::drown_uniform(random, hmpc::constant_of<Bound>, hmpc::constant_of<Signedness>, hmpc::constant_of<StatisticalSecurity>));
        }
    };

    template<hmpc::value T, auto Tag = []{}, auto Bound, hmpc::statistical_security StatisticalSecurity = default_statistical_security, hmpc::size... Dimensions>
    constexpr auto drown_unsigned_uniform(hmpc::shape<Dimensions...> shape, hmpc::constant<decltype(Bound), Bound> = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> = {}) noexcept
    {
        return drown_uniform_expression<T, Bound, hmpc::without_sign, Tag, StatisticalSecurity, Dimensions...>{shape};
    }

    template<hmpc::value T, auto Tag = []{}, auto Bound, hmpc::statistical_security StatisticalSecurity = default_statistical_security, hmpc::size... Dimensions>
    constexpr auto drown_unsigned_uniform(hmpc::expr::random::use_number_generator<Tag>, hmpc::shape<Dimensions...> shape, hmpc::constant<decltype(Bound), Bound> = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> = {}) noexcept
    {
        return drown_uniform_expression<T, Bound, hmpc::without_sign, Tag, StatisticalSecurity, Dimensions...>{shape};
    }

    template<hmpc::value T, auto Tag = []{}, auto Bound, hmpc::statistical_security StatisticalSecurity = default_statistical_security, hmpc::size... Dimensions>
    constexpr auto drown_signed_uniform(hmpc::shape<Dimensions...> shape, hmpc::constant<decltype(Bound), Bound> = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> = {}) noexcept
    {
        return drown_uniform_expression<T, Bound, hmpc::with_sign, Tag, StatisticalSecurity, Dimensions...>{shape};
    }

    template<hmpc::value T, auto Tag = []{}, auto Bound, hmpc::statistical_security StatisticalSecurity = default_statistical_security, hmpc::size... Dimensions>
    constexpr auto drown_signed_uniform(hmpc::expr::random::use_number_generator<Tag>, hmpc::shape<Dimensions...> shape, hmpc::constant<decltype(Bound), Bound> = {}, hmpc::constant<hmpc::statistical_security, StatisticalSecurity> = {}) noexcept
    {
        return drown_uniform_expression<T, Bound, hmpc::with_sign, Tag, StatisticalSecurity, Dimensions...>{shape};
    }
}
