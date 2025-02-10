#pragma once

#include <hmpc/comp/accessor.hpp>
#include <hmpc/detail/type_tag.hpp>
#include <hmpc/expr/expression.hpp>
#include <hmpc/shape.hpp>

#define HMPC_BINARY_EXPRESSION(NAME, OP) \
    namespace traits \
    { \
        template<typename Left, typename Right> \
        struct NAME##_specialization \
        { \
        }; \
    } \
    template<typename Left, typename Right> \
    concept NAME##_is_specialized = requires \
    { \
        traits::NAME##_specialization<Left, Right>::is_specialized; \
    }; \
\
    template<hmpc::expression Left, hmpc::expression Right> \
    struct NAME##_expression \
    { \
        using left_type = Left; \
        using right_type = Right; \
        using left_value_type = left_type::value_type; \
        using right_value_type = right_type::value_type; \
        using value_type = std::remove_cvref_t<decltype(std::declval<left_value_type>() OP std::declval<right_value_type>())>; \
        using element_type = hmpc::traits::element_type_t<value_type>; \
\
        using left_shape_type = left_type::shape_type; \
        using right_shape_type = right_type::shape_type; \
\
        static constexpr hmpc::size arity = 2; \
\
        left_type left; \
        right_type right; \
\
        constexpr NAME##_expression(left_type left, right_type right) HMPC_NOEXCEPT \
            : left(left) \
            , right(right) \
        { \
        } \
\
        constexpr left_type const& get(hmpc::size_constant<0>) const HMPC_NOEXCEPT \
        { \
            return left; \
        } \
\
        constexpr right_type const& get(hmpc::size_constant<1>) const HMPC_NOEXCEPT \
        { \
            return right; \
        } \
\
        static constexpr auto access(hmpc::size_constant<0>) noexcept \
        { \
            if constexpr (left_shape_type::rank != right_shape_type::rank) \
            { \
                return hmpc::access::multiple; /* TODO: do not have to return `multiple` if other one is just placeholders */ \
            } \
            else if constexpr (left_shape_type::rank == 0) \
            { \
                return hmpc::access::once; /* right has same rank by the test above */ \
            } \
            else \
            { \
                return hmpc::iter::scan_range<left_shape_type::rank>([](auto i, auto pattern) \
                { \
                    constexpr auto extent = left_shape_type::extent(i); \
                    if constexpr (extent == hmpc::placeholder_extent and right_shape_type::extent(i) != extent) \
                    { \
                        return hmpc::access::multiple; \
                    } \
                    else \
                    { \
                        return pattern; \
                    } \
                }, hmpc::access::once); \
            } \
        } \
\
        static constexpr auto access(hmpc::size_constant<1>) noexcept \
        { \
            if constexpr (left_shape_type::rank != right_shape_type::rank) \
            { \
                return hmpc::access::multiple; /* TODO: do not have to return `multiple` if other one is just placeholders */ \
            } \
            else if constexpr (right_shape_type::rank == 0) \
            { \
                return hmpc::access::once; /* left has same rank by the test above */ \
            } \
            else \
            { \
                return hmpc::iter::scan_range<right_shape_type::rank>([](auto i, auto pattern) \
                { \
                    constexpr auto extent = right_shape_type::extent(i); \
                    if constexpr (extent == hmpc::placeholder_extent and left_shape_type::extent(i) != extent) \
                    { \
                        return hmpc::access::multiple; \
                    } \
                    else \
                    { \
                        return pattern; \
                    } \
                }, hmpc::access::once); \
            } \
        } \
\
        constexpr auto shape() const HMPC_NOEXCEPT \
        { \
            return hmpc::common_shape(left.shape(), right.shape()); \
        } \
\
        using shape_type = decltype(hmpc::common_shape(std::declval<left_shape_type>(), std::declval<right_shape_type>())); \
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>; \
\
        static constexpr element_type operator()(hmpc::state_with_arity<2> auto const& state, hmpc::index_for<element_shape_type> auto const& index, auto& capabilities) HMPC_NOEXCEPT \
        { \
            if constexpr (NAME##_is_specialized<left_value_type, right_value_type>) \
            { \
                return traits::NAME##_specialization<left_value_type, right_value_type>::operator()( \
                    hmpc::detail::tag_of<left_type>, \
                    state.get(hmpc::constants::zero), \
                    index, \
                    hmpc::detail::tag_of<right_type>, \
                    state.get(hmpc::constants::one), \
                    index, \
                    capabilities \
                ); \
            } \
            else \
            { \
                return left_type::operator()(state.get(hmpc::constants::zero), index, capabilities) OP right_type::operator()(state.get(hmpc::constants::one), index, capabilities); \
            } \
        } \
    }; \
\
    namespace operators \
    { \
        template<typename Left, typename Right> \
            requires (hmpc::expression<std::remove_cvref_t<Left>> and hmpc::expression<std::remove_cvref_t<Right>>) \
        constexpr auto operator OP(Left&& left, Right&& right) HMPC_NOEXCEPT \
        { \
            return NAME##_expression(std::forward<Left>(left), std::forward<Right>(right)); \
        } \
    }

namespace hmpc::expr
{
    HMPC_BINARY_EXPRESSION(add, +);
    HMPC_BINARY_EXPRESSION(subtract, -);
    HMPC_BINARY_EXPRESSION(multiply, *);
    HMPC_BINARY_EXPRESSION(bit_and, &);
    HMPC_BINARY_EXPRESSION(bit_or, |);
    HMPC_BINARY_EXPRESSION(bit_xor, ^);
    HMPC_BINARY_EXPRESSION(equal_to, ==);
    HMPC_BINARY_EXPRESSION(not_equal_to, !=);
    HMPC_BINARY_EXPRESSION(less, <);
    HMPC_BINARY_EXPRESSION(greater, >);
    HMPC_BINARY_EXPRESSION(less_equal, <=);
    HMPC_BINARY_EXPRESSION(greater_equal, >=);
}

#undef HMPC_BINARY_EXPRESSION
