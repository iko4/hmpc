#pragma once

#include <hmpc/expr/binary_expression.hpp>

namespace hmpc::expr
{
    template<typename Left, typename Right>
    constexpr auto matrix_product_shape(Left left, Right right) HMPC_NOEXCEPT
    {
        constexpr auto left_rank = Left::rank;
        constexpr auto right_rank = Right::rank;
        static_assert(left_rank == right_rank);
        static_assert(left_rank >= 2);
        constexpr auto u_index = hmpc::size_constant_of<left_rank - 2>;
        constexpr auto v_index = hmpc::size_constant_of<right_rank - 1>;
        static_assert(Left::extent(u_index) != hmpc::placeholder_extent);
        static_assert(Right::extent(v_index) != hmpc::placeholder_extent);

        return hmpc::iter::for_packed_range<left_rank - 2>([&](auto... i)
        {
            return hmpc::shape{hmpc::common_extent(left.get(i), right.get(i))..., left.get(u_index), right.get(v_index)};
        });
    }

    template<hmpc::expression Left, hmpc::expression Right>
    struct matrix_product_expression
    {
        using left_type = Left;
        using right_type = Right;

        using left_shape_type = left_type::shape_type;
        using right_shape_type = right_type::shape_type;

        static constexpr hmpc::size left_rank = left_shape_type::rank;
        static constexpr hmpc::size right_rank = right_shape_type::rank;
        static_assert(left_rank == right_rank);
        static_assert(left_rank >= 2);
        static constexpr hmpc::size sum_extent = []()
        {
            constexpr auto u_index = hmpc::size_constant_of<left_rank - 2>;
            constexpr auto v_index = hmpc::size_constant_of<right_rank - 1>;
            constexpr auto extent = left_shape_type::extent(v_index);
            static_assert(extent == right_shape_type::extent(u_index));
            static_assert(extent != hmpc::placeholder_extent);
            static_assert(extent != hmpc::dynamic_extent);
            return extent;
        }();

        using left_value_type = left_type::value_type;
        using right_value_type = right_type::value_type;
        using multiply_type = std::remove_cvref_t<decltype(std::declval<left_value_type>() * std::declval<right_value_type>())>;
        using value_type = std::remove_cvref_t<typename decltype(hmpc::iter::for_packed_range<sum_extent>([](auto... i){ using T = decltype(((static_cast<void>(i), std::declval<multiply_type>()) + ...)); return hmpc::detail::tag_of<T>; }))::type>;
        using element_type = hmpc::traits::element_type_t<value_type>;

        static constexpr hmpc::size arity = 2;

        left_type left;
        right_type right;

        constexpr matrix_product_expression(left_type left, right_type right) HMPC_NOEXCEPT
            : left(left)
            , right(right)
        {
        }

        constexpr left_type const& get(hmpc::size_constant<0>) const HMPC_NOEXCEPT
        {
            return left;
        }

        constexpr right_type const& get(hmpc::size_constant<1>) const HMPC_NOEXCEPT
        {
            return right;
        }

        static constexpr hmpc::access::multiple_tag access(hmpc::size_constant<0>) noexcept
        {
            return {};
        }

        static constexpr hmpc::access::multiple_tag access(hmpc::size_constant<1>) noexcept
        {
            return {};
        }

        constexpr auto shape() const HMPC_NOEXCEPT
        {
            return matrix_product_shape(left.shape(), right.shape());
        }

        using shape_type = decltype(matrix_product_shape(std::declval<left_shape_type>(), std::declval<right_shape_type>()));
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        template<hmpc::index_for<element_shape_type> Index>
        static constexpr element_type operator()(hmpc::state_with_arity<2> auto const& state, Index const& index, auto& capabilities) HMPC_NOEXCEPT
        {
            return hmpc::iter::for_packed_range<sum_extent>([&](auto... k)
            {
                return ([&]()
                {
                    constexpr auto index_rank = Index::rank;
                    auto left_index = hmpc::iter::for_packed_range<left_rank - 1>([&](auto... i)
                    {
                        return hmpc::iter::for_packed_range<left_rank, index_rank>([&](auto... j)
                        {
                            return hmpc::index{index.get(i)..., k, index.get(j)...};
                        });
                    });

                    auto right_index = hmpc::iter::for_packed_range<right_rank - 2>([&](auto... i)
                    {
                        return hmpc::iter::for_packed_range<right_rank - 1, index_rank>([&](auto... j)
                        {
                            return hmpc::index{index.get(i)..., k, index.get(j)...};
                        });
                    });

                    if constexpr (multiply_is_specialized<left_value_type, right_value_type>)
                    {
                        return traits::multiply_specialization<left_value_type, right_value_type>::operator()(
                            hmpc::detail::tag_of<left_type>,
                            state.get(hmpc::constants::zero),
                            left_index,
                            hmpc::detail::tag_of<right_type>,
                            state.get(hmpc::constants::one),
                            right_index,
                            capabilities
                        );
                    }
                    else
                    {
                        return left_type::operator()(state.get(hmpc::constants::zero), left_index, capabilities) * right_type::operator()(state.get(hmpc::constants::one), right_index, capabilities);
                    }
                }() + ...);
            });
        }
    };

    template<hmpc::expression Left, hmpc::expression Right>
    constexpr auto matrix_product(Left left, Right right) HMPC_NOEXCEPT
    {
        return matrix_product_expression<Left, Right>{left, right};
    }
}
