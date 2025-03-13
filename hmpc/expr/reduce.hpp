#pragma once

#include <hmpc/expr/expression.hpp>
#include <hmpc/ints/integer_traits.hpp>
#include <hmpc/shape.hpp>

namespace hmpc::reduction
{
    struct add_tag
    {
    };
    constexpr add_tag add = {};

    struct multiply_tag
    {
    };
    constexpr multiply_tag multiply = {};

    struct logical_and_tag
    {
    };
    constexpr logical_and_tag logical_and = {};

    struct logical_or_tag
    {
    };
    constexpr logical_or_tag logical_or = {};

    struct bit_and_tag
    {
    };
    constexpr bit_and_tag bit_and = {};

    struct bit_or_tag
    {
    };
    constexpr bit_or_tag bit_or = {};

    struct bit_xor_tag
    {
    };
    constexpr bit_xor_tag bit_xor = {};

    struct min_tag
    {
    };
    constexpr min_tag min = {};

    struct max_tag
    {
    };
    constexpr max_tag max = {};
}

namespace hmpc::expr
{
    namespace detail
    {
        template<typename Operation>
        struct reduction_type;

        template<>
        struct reduction_type<hmpc::reduction::add_tag>
        {
            using type = sycl::plus<>;
        };

        template<>
        struct reduction_type<hmpc::reduction::multiply_tag>
        {
            using type = sycl::multiplies<>;
        };

        template<>
        struct reduction_type<hmpc::reduction::logical_and_tag>
        {
            using type = sycl::logical_and<>;
        };

        template<>
        struct reduction_type<hmpc::reduction::logical_or_tag>
        {
            using type = sycl::logical_or<>;
        };

        template<>
        struct reduction_type<hmpc::reduction::bit_and_tag>
        {
            using type = sycl::bit_and<>;
        };

        template<>
        struct reduction_type<hmpc::reduction::bit_or_tag>
        {
            using type = sycl::bit_or<>;
        };

        template<>
        struct reduction_type<hmpc::reduction::bit_xor_tag>
        {
            using type = sycl::bit_xor<>;
        };

        template<>
        struct reduction_type<hmpc::reduction::min_tag>
        {
            using type = sycl::minimum<>;
        };

        template<>
        struct reduction_type<hmpc::reduction::max_tag>
        {
            using type = sycl::maximum<>;
        };

        template<typename Operation>
        constexpr auto reduction_type_v = typename reduction_type<Operation>::type{};

        template<typename Operation, typename T>
        struct reduction_identity;

        template<typename T>
        struct reduction_identity<hmpc::reduction::add_tag, T>
        {
            using type = T;
            static constexpr auto value = hmpc::ints::integer_traits<type>::zero;
        };

        template<typename T>
        struct reduction_identity<hmpc::reduction::multiply_tag, T>
        {
            using type = T;
            static constexpr auto value = hmpc::ints::integer_traits<type>::one;
        };

        template<>
        struct reduction_identity<hmpc::reduction::logical_and_tag, hmpc::bit>
        {
            using type = hmpc::bit;
            static constexpr auto value = type{true};
        };

        template<>
        struct reduction_identity<hmpc::reduction::logical_or_tag, hmpc::bit>
        {
            using type = hmpc::bit;
            static constexpr auto value = type{false};
        };

        template<typename T>
        struct reduction_identity<hmpc::reduction::bit_and_tag, T>
        {
            using type = T;
            static constexpr auto value = hmpc::ints::integer_traits<type>::all_ones;
        };

        template<typename T>
        struct reduction_identity<hmpc::reduction::bit_or_tag, T>
        {
            using type = T;
            static constexpr auto value = hmpc::ints::integer_traits<type>::all_zeros;
        };

        template<typename T>
        struct reduction_identity<hmpc::reduction::bit_xor_tag, T>
        {
            using type = T;
            static constexpr auto value = hmpc::ints::integer_traits<type>::all_zeros;
        };

        template<typename T>
        struct reduction_identity<hmpc::reduction::min_tag, T>
        {
            using type = T;
            static constexpr auto value = hmpc::ints::integer_traits<type>::max;
        };

        template<typename T>
        struct reduction_identity<hmpc::reduction::max_tag, T>
        {
            using type = T;
            static constexpr auto value = hmpc::ints::integer_traits<type>::min;
        };

        template<typename Operation, typename T>
        constexpr auto reduction_identity_v = reduction_identity<Operation, T>::value;
    }

    template<hmpc::expression E, typename Operation>
    struct reduction_expression : public enable_caching
    {
        using enable_caching::operator();

        using inner_type = E;
        using value_type = inner_type::element_type;
        using element_type = value_type;
        using limb_type = hmpc::traits::limb_type_t<value_type>;
        using shape_type = hmpc::shape<>;

        using operation_type = Operation;

        static constexpr auto arity = hmpc::size_constant_of<1>;
        using is_complex = void;

        inner_type inner;

        constexpr reduction_expression(inner_type inner) noexcept
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

        constexpr shape_type shape() const noexcept
        {
            return {};
        }

        constexpr auto operator()(auto& sycl_queue, auto& get_state, auto& get_capability_data, auto& make_capabilities, auto& tensor, auto&) const HMPC_NOEXCEPT
        {
            return sycl_queue.submit([&](auto& handler)
            {
                auto state = get_state(handler).get(hmpc::constants::zero);
                auto element_shape = hmpc::expr::element_shape(inner);
                auto capability_data = get_capability_data(element_shape);

                auto reduction = sycl::reduction(tensor.get(), handler, detail::reduction_identity_v<operation_type, element_type>, detail::reduction_type_v<operation_type>, sycl::property::reduction::initialize_to_identity());

                handler.parallel_for(sycl::range{element_shape.size()}, reduction, [=](hmpc::size i, auto& partial)
                {
                    auto index = [&]()
                    {
                        if constexpr (hmpc::expr::same_element_shape<inner_type>)
                        {
                            return i;
                        }
                        else
                        {
                            return hmpc::from_linear_index(i, element_shape);
                        }
                    }();

                    auto capabilities = make_capabilities(capability_data, index, element_shape);

                    partial.combine(inner_type::operator()(state, index, capabilities));
                });
            });
        }
    };

    template<typename Operation, hmpc::expression E>
    constexpr auto reduce(E expression, Operation) noexcept
    {
        return reduction_expression<E, Operation>{expression};
    }

    template<hmpc::expression E>
    constexpr auto sum(E expression) noexcept
    {
        return reduce(expression, hmpc::reduction::add);
    }

    template<hmpc::expression E>
    constexpr auto product(E expression) noexcept
    {
        return reduce(expression, hmpc::reduction::multiply);
    }

    template<hmpc::expression E>
    constexpr auto all(E expression) noexcept
    {
        return reduce(expression, hmpc::reduction::logical_and);
    }

    template<hmpc::expression E>
    constexpr auto any(E expression) noexcept
    {
        return reduce(expression, hmpc::reduction::logical_or);
    }

    template<hmpc::expression E>
    constexpr auto min(E expression) noexcept
    {
        return reduce(expression, hmpc::reduction::min);
    }

    template<hmpc::expression E>
    constexpr auto max(E expression) noexcept
    {
        return reduce(expression, hmpc::reduction::max);
    }
}
