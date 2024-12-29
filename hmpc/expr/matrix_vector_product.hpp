#pragma once

#include <hmpc/expr/binary_expression.hpp>

namespace hmpc::expr
{
    template<typename Matrix, typename Vector>
    constexpr auto matrix_vector_product_shape(Matrix matrix, Vector vector) HMPC_NOEXCEPT
    {
        constexpr auto matrix_rank = Matrix::rank;
        constexpr auto vector_rank = Vector::rank;
        static_assert(matrix_rank == vector_rank + 1);
        static_assert(matrix_rank >= 2);
        constexpr auto u_index = hmpc::size_constant_of<matrix_rank - 2>;
        constexpr auto v_index = hmpc::size_constant_of<matrix_rank - 1>;
        static_assert(Matrix::extent(u_index) != hmpc::placeholder_extent);
        static_assert(Matrix::extent(v_index) != hmpc::placeholder_extent);
        static_assert(Vector::extent(u_index) != hmpc::placeholder_extent);

        return hmpc::iter::for_packed_range<matrix_rank - 2>([&](auto... i)
        {
            return hmpc::shape{hmpc::common_extent(matrix.get(i), vector.get(i))..., matrix.get(u_index)};
        });
    }

    template<hmpc::expression Matrix, hmpc::expression Vector>
    struct matrix_vector_product_expression
    {
        using matrix_type = Matrix;
        using vector_type = Vector;

        using matrix_shape_type = matrix_type::shape_type;
        using vector_shape_type = vector_type::shape_type;

        static constexpr hmpc::size matrix_rank = matrix_shape_type::rank;
        static constexpr hmpc::size vector_rank = vector_shape_type::rank;
        static_assert(matrix_rank == vector_rank + 1);
        static_assert(matrix_rank >= 2);
        static constexpr hmpc::size sum_extent = []()
        {
            constexpr auto u_index = hmpc::size_constant_of<matrix_rank - 2>;
            constexpr auto v_index = hmpc::size_constant_of<matrix_rank - 1>;
            constexpr auto extent = matrix_shape_type::extent(v_index);
            static_assert(extent == vector_shape_type::extent(u_index));
            static_assert(extent != hmpc::placeholder_extent);
            static_assert(extent != hmpc::dynamic_extent);
            return extent;
        }();

        using matrix_value_type = matrix_type::value_type;
        using vector_value_type = vector_type::value_type;
        using multiply_type = std::remove_cvref_t<decltype(std::declval<matrix_value_type>() * std::declval<vector_value_type>())>;
        using value_type = std::remove_cvref_t<typename decltype(hmpc::iter::for_packed_range<sum_extent>([](auto... i){ using T = decltype(((static_cast<void>(i), std::declval<multiply_type>()) + ...)); return hmpc::detail::tag_of<T>; }))::type>;
        using element_type = hmpc::traits::element_type_t<value_type>;

        static constexpr hmpc::size arity = 2;

        matrix_type matrix;
        vector_type vector;

        constexpr matrix_vector_product_expression(matrix_type matrix, vector_type vector) HMPC_NOEXCEPT
            : matrix(matrix)
            , vector(vector)
        {
        }

        constexpr matrix_type const& get(hmpc::size_constant<0>) const HMPC_NOEXCEPT
        {
            return matrix;
        }

        constexpr vector_type const& get(hmpc::size_constant<1>) const HMPC_NOEXCEPT
        {
            return vector;
        }

        static constexpr hmpc::access::once_tag access(hmpc::size_constant<0>) noexcept
        {
            return {};
        }

        static constexpr hmpc::access::multiple_tag access(hmpc::size_constant<1>) noexcept
        {
            return {};
        }

        constexpr auto shape() const HMPC_NOEXCEPT
        {
            return matrix_vector_product_shape(matrix.shape(), vector.shape());
        }

        using shape_type = decltype(matrix_vector_product_shape(std::declval<matrix_shape_type>(), std::declval<vector_shape_type>()));
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        template<hmpc::index_for<element_shape_type> Index>
        static constexpr element_type operator()(hmpc::state_with_arity<2> auto const& state, Index const& index, auto& capabilities) HMPC_NOEXCEPT
        {
            return hmpc::iter::for_packed_range<sum_extent>([&](auto... k)
            {
                return ([&]()
                {
                    constexpr auto index_rank = Index::rank;
                    auto matrix_index = hmpc::iter::for_packed_range<matrix_rank - 1>([&](auto... i)
                    {
                        return hmpc::iter::for_packed_range<matrix_rank - 1, index_rank>([&](auto... j)
                        {
                            return hmpc::index{index.get(i)..., k, index.get(j)...};
                        });
                    });

                    auto vector_index = hmpc::iter::for_packed_range<vector_rank - 1>([&](auto... i)
                    {
                        return hmpc::iter::for_packed_range<vector_rank, index_rank>([&](auto... j)
                        {
                            return hmpc::index{index.get(i)..., k, index.get(j)...};
                        });
                    });

                    if constexpr (multiply_is_specialized<matrix_value_type, vector_value_type>)
                    {
                        return traits::multiply_specialization<matrix_value_type, vector_value_type>::operator()(
                            hmpc::detail::tag_of<matrix_type>,
                            state.get(hmpc::constants::zero),
                            matrix_index,
                            hmpc::detail::tag_of<vector_type>,
                            state.get(hmpc::constants::one),
                            vector_index,
                            capabilities
                        );
                    }
                    else
                    {
                        return matrix_type::operator()(state.get(hmpc::constants::zero), matrix_index, capabilities) * vector_type::operator()(state.get(hmpc::constants::one), vector_index, capabilities);
                    }
                }() + ...);
            });
        }
    };

    template<hmpc::expression Matrix, hmpc::expression Vector>
    constexpr auto matrix_vector_product(Matrix matrix, Vector vector) HMPC_NOEXCEPT
    {
        return matrix_vector_product_expression<Matrix, Vector>{matrix, vector};
    }
}
