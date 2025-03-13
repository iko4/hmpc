#pragma once

#include <hmpc/config.hpp>
#include <hmpc/core/equal_to.hpp>
#include <hmpc/core/mask_inside.hpp>
#include <hmpc/core/shift_right.hpp>
#include <hmpc/detail/utility.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/ints/bit_monomial.hpp>
#include <hmpc/ints/integer_traits.hpp>
#include <hmpc/ints/poly.hpp>

namespace hmpc::expr
{
    namespace traits
    {
        template<hmpc::size Degree, typename Polynomial>
        struct multiply_specialization<hmpc::ints::bit_monomial<Degree>, Polynomial>
        {
            static constexpr bool is_specialized = true;

            template<typename Left, typename LeftState, hmpc::mdindex_for<hmpc::expr::traits::element_shape_t<Left>> LeftIndex, typename Right, typename RightState, hmpc::mdindex_for<hmpc::expr::traits::element_shape_t<Right>> RightIndex, typename Capabilities>
            static constexpr auto operator()(hmpc::detail::type_tag<Left>, LeftState const& left_state, LeftIndex const& left_index, hmpc::detail::type_tag<Right>, RightState const& right_state, RightIndex const& right_index, Capabilities const& capabilities) HMPC_NOEXCEPT
                requires (std::same_as<typename Left::value_type, hmpc::ints::bit_monomial<Degree>> and std::same_as<typename Right::value_type, Polynomial>)
            {
                using left_type = Left;
                using right_type = Right;

                using element_type = decltype(-std::declval<typename right_type::element_type>());

                constexpr auto max_degree = hmpc::traits::vector_size_v<left_type>;
                static_assert(hmpc::traits::vector_size_v<typename right_type::value_type> == max_degree);

                auto monomial = left_type::operator()(left_state, left_index, capabilities);

                if (auto const& degree = monomial.degree; degree.has_value())
                {
                    static_assert(RightIndex::rank > 0);
                    constexpr auto element_dimension = hmpc::size_constant_of<RightIndex::rank - 1>;
                    static_assert(hmpc::expr::traits::element_shape_t<right_type>::extent(element_dimension) == max_degree);
                    auto [shifted_index, is_negative] = [element_dimension, degree = *degree, element_index = right_index.get(element_dimension)](auto index)
                    {
                        // output_index = element_index = input_index - degree mod N
                        // [0, 1, ..., N - 1, N, N + 1, ..., 2N - 1, 2N, 2N + 1, ..., 3N - 2]
                        //  ^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^
                        //      same sign         inverted sign             same sign
                        //
                        // The following is equivalent to:
                        // ```
                        // if (element_index + max_degree < degree)
                        // {
                        //     HMPC_ASSERT(element_index + max_degree + max_degree >= degree);
                        //     index.get(element_dimension) = element_index + max_degree + max_degree - degree;
                        //     return std::make_pair(index, false);
                        // }
                        // else if (element_index < degree)
                        // {
                        //     index.get(element_dimension) = element_index + max_degree - degree;
                        //     return std::make_pair(index, true);
                        // }
                        // else
                        // {
                        //     index.get(element_dimension) = element_index - degree;
                        //     return std::make_pair(index, false);
                        // }
                        // ```
                        static_assert(hmpc::detail::has_single_bit(max_degree));
                        constexpr auto bits = hmpc::size_constant_of<hmpc::detail::countr_zero(max_degree)>;
                        static_assert(max_degree == 1 << bits);
                        static_assert(not std::numeric_limits<hmpc::size>::is_signed);

                        auto difference = element_index - degree;

                        index.get(element_dimension) = hmpc::core::mask_inside(difference, bits); // = difference mod max_degree

                        auto flag = hmpc::core::bit_and(               // masking with 1 to find if we had exactly one underflow (not zero or two)
                            hmpc::core::shift_right(difference, bits), // = difference div max_degree
                            hmpc::constants::one
                        );

                        return std::make_pair(index, hmpc::core::equal_to(flag, hmpc::constants::one));
                    }(right_index);
                    auto element = right_type::operator()(right_state, shifted_index, capabilities);
                    if (is_negative)
                    {
                        return -element;
                    }
                    else
                    {
                        if constexpr (std::same_as<element_type, typename right_type::element_type>)
                        {
                            return element;
                        }
                        else
                        {
                            return static_cast<element_type>(element);
                        }
                    }
                }
                else
                {
                    return hmpc::ints::integer_traits<element_type>::zero;
                }
            }
        };

        template<hmpc::size Degree, typename Polynomial>
        struct multiply_specialization<Polynomial, hmpc::ints::bit_monomial<Degree>>
        {
            static constexpr bool is_specialized = true;

            template<typename Left, typename LeftState, hmpc::mdindex_for<hmpc::expr::traits::element_shape_t<Left>> LeftIndex, typename Right, typename RightState, hmpc::mdindex_for<hmpc::expr::traits::element_shape_t<Right>> RightIndex, typename Capabilities>
            static constexpr auto operator()(hmpc::detail::type_tag<Left> left, LeftState const& left_state, LeftIndex const& left_index, hmpc::detail::type_tag<Right> right, RightState const& right_state, RightIndex const& right_index, Capabilities const& capabilities) HMPC_NOEXCEPT
            {
                return multiply_specialization<hmpc::ints::bit_monomial<Degree>, Polynomial>::operator()(right, right_state, right_index, left, left_state, left_index, capabilities);
            }
        };
    }

    template<hmpc::size Degree, hmpc::expression E>
        requires (std::same_as<typename E::value_type, std::optional<hmpc::size>>)
    struct bit_monomial_expression
    {
        using inner_type = E;
        using value_type = hmpc::ints::bit_monomial<Degree>;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = inner_type::shape_type;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr auto vector_size = hmpc::traits::vector_size_v<value_type>;

        static constexpr auto arity = hmpc::size_constant_of<1>;

        inner_type inner;

        constexpr bit_monomial_expression(inner_type inner) HMPC_NOEXCEPT
            : inner(inner)
        {
        }

        constexpr inner_type const& get(hmpc::size_constant<0>) const noexcept
        {
            return inner;
        }

        static constexpr hmpc::access::multiple_tag access(hmpc::size_constant<0>) noexcept
        {
            return {};
        }

        constexpr decltype(auto) shape() const HMPC_NOEXCEPT
        {
            return inner.shape();
        }

        static constexpr element_type operator()(hmpc::state_with_arity<1> auto const& state, hmpc::mdindex_for<element_shape_type> auto const& index, auto& capabilities) HMPC_NOEXCEPT
        {
            return element_type{inner_type::operator()(state.get(hmpc::constants::zero), hmpc::unsqueeze(index, hmpc::constants::minus_one, hmpc::force), capabilities)};
        }
    };

    template<hmpc::size Degree, hmpc::expression E>
    constexpr auto bit_monomial(E expr) HMPC_NOEXCEPT
    {
        return bit_monomial_expression<Degree, E>{expr};
    }
}
