#pragma once

#include <hmpc/expr/random/number_generator.hpp>
#include <hmpc/random/binomial.hpp>
#include <hmpc/random/number_generator.hpp>
#include <hmpc/shape.hpp>
#include <hmpc/value.hpp>

namespace hmpc::expr::random
{
    template<hmpc::value T, hmpc::size Count, auto Tag = []{}, hmpc::size... Dimensions>
    struct binomial_expression : public enable_caching
    {
        using enable_caching::operator();

        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = hmpc::shape<Dimensions...>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr hmpc::size arity = 0;

        using capabilities = hmpc::detail::type_list<hmpc::expr::capabilities::random_number_generator_tag>;

        shape_type HMPC_PRIVATE_MEMBER(shape);

        constexpr binomial_expression(shape_type shape) HMPC_NOEXCEPT
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
            return static_cast<element_type>(hmpc::random::binomial(random, hmpc::size_constant_of<Count>));
        }
    };

    template<hmpc::value T, hmpc::size Count, auto Tag = []{}, hmpc::size... Dimensions>
    constexpr auto binomial(hmpc::shape<Dimensions...> shape) noexcept
    {
        return binomial_expression<T, Count, Tag, Dimensions...>{shape};
    }

    template<hmpc::value T, hmpc::rational_size Variance, auto Tag = []{}, hmpc::size... Dimensions>
    struct centered_binomial_expression : public enable_caching
    {
        using enable_caching::operator();

        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = hmpc::shape<Dimensions...>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr hmpc::size arity = 0;

        using capabilities = hmpc::detail::type_list<hmpc::expr::capabilities::random_number_generator_tag>;

        shape_type HMPC_PRIVATE_MEMBER(shape);

        constexpr centered_binomial_expression(shape_type shape) HMPC_NOEXCEPT
            : HMPC_PRIVATE_MEMBER(shape)(shape)
        {
        }

        constexpr shape_type const& shape() const noexcept
        {
            return HMPC_PRIVATE_MEMBER(shape);
        }

        constexpr hmpc::monostate state(auto&) const noexcept
        {
            return hmpc::empty;
        }

        static constexpr element_type operator()(hmpc::monostate, hmpc::index_for<element_shape_type> auto const&, auto& capabilities) HMPC_DEVICE_NOEXCEPT
        {
            auto& random = capabilities.get(hmpc::expr::capabilities::random_number_generator);
            return static_cast<element_type>(hmpc::random::centered_binomial(random, hmpc::constant_of<Variance>));
        }
    };

    template<hmpc::value T, hmpc::size Variance, auto Tag = []{}, hmpc::size... Dimensions>
    constexpr auto centered_binomial(hmpc::shape<Dimensions...> shape, hmpc::size_constant<Variance> = {}) noexcept
    {
        return centered_binomial_expression<T, Variance, Tag, Dimensions...>{shape};
    }

    template<hmpc::value T, hmpc::size Variance, auto Tag = []{}, hmpc::size... Dimensions>
    constexpr auto centered_binomial(hmpc::expr::random::use_number_generator<Tag>, hmpc::shape<Dimensions...> shape, hmpc::size_constant<Variance> = {}) noexcept
    {
        return centered_binomial_expression<T, Variance, Tag, Dimensions...>{shape};
    }

    template<hmpc::value T, hmpc::rational_size Variance, auto Tag = []{}, hmpc::size... Dimensions>
    constexpr auto centered_binomial(hmpc::shape<Dimensions...> shape, hmpc::rational_size_constant<Variance> = {}) noexcept
    {
        return centered_binomial_expression<T, Variance, Tag, Dimensions...>{shape};
    }

    template<hmpc::value T, hmpc::rational_size Variance, auto Tag = []{}, hmpc::size... Dimensions>
    constexpr auto centered_binomial(hmpc::expr::random::use_number_generator<Tag>, hmpc::shape<Dimensions...> shape, hmpc::rational_size_constant<Variance> = {}) noexcept
    {
        return centered_binomial_expression<T, Variance, Tag, Dimensions...>{shape};
    }
}
