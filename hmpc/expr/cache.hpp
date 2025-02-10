#pragma once

#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/tensor.hpp>
#include <hmpc/constants.hpp>
#include <hmpc/detail/type_map.hpp>
#include <hmpc/detail/type_set.hpp>
#include <hmpc/detail/utility.hpp>
#include <hmpc/expr/expression.hpp>

namespace hmpc
{
    template<typename T>
    concept cacheable_expression = expression<T> and requires
    {
        typename T::is_cacheable;
    };
}

namespace hmpc::expr
{
    struct enable_caching
    {
        using is_cacheable = void;

        template<hmpc::accessor Accessor>
        static constexpr typename Accessor::element_type operator()(Accessor const& accessor, hmpc::index_for<typename Accessor::element_shape_type> auto const& index, auto const&) HMPC_NOEXCEPT
        {
            return accessor[index];
        }
    };

    template<hmpc::expression E>
    struct cache_expression : public enable_caching
    {
        using inner_type = E;
        using value_type = inner_type::value_type;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = inner_type::shape_type;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr hmpc::size arity = 1;

        inner_type inner;

        constexpr cache_expression(inner_type inner) HMPC_NOEXCEPT
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

        constexpr decltype(auto) shape() const HMPC_NOEXCEPT
        {
            return inner.shape();
        }

        static constexpr element_type operator()(hmpc::state_with_arity<1> auto const& state, hmpc::index_for<element_shape_type> auto const& index, auto& capabilities) HMPC_NOEXCEPT
        {
            return inner_type::operator()(state.get(hmpc::constants::zero), index, capabilities);
        }
    };

    template<hmpc::expression E>
    constexpr auto cache(E expression) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::cacheable_expression<E>)
        {
            return expression;
        }
        else
        {
            return cache_expression<E>{expression};
        }
    }

    namespace detail
    {
        template<hmpc::expression E>
        constexpr decltype(auto) trace_expression(auto&& traces, auto trace, E expr) HMPC_HOST_NOEXCEPT
        {
            if constexpr (hmpc::cacheable_expression<E>)
            {
                return std::forward<decltype(traces)>(traces).insert_or_assign(
                    hmpc::detail::tag_of<E>,
                    traces.get_or(hmpc::detail::tag_of<E>, hmpc::detail::type_set{}).insert(trace)
                );
            }
            else if constexpr (expr.arity > 0)
            {
                return hmpc::iter::scan_range<expr.arity>([&](auto i, auto&& traces)
                {
                    return trace_expression(
                        std::forward<decltype(traces)>(traces),
                        trace.append(hmpc::detail::triple_tag_of<E, decltype(E::access(i)), decltype(i)>), expr.get(i)
                    );
                }, std::forward<decltype(traces)>(traces));
            }
            else
            {
                return std::forward<decltype(traces)>(traces);
            }
        }

        template<hmpc::expression E>
        constexpr auto find_next_expression_to_trace(auto&& traces, E expr) HMPC_HOST_NOEXCEPT
        {
            if constexpr (expr.arity > 0)
            {
                auto traverse_children = [&]()
                {
                    return hmpc::iter::scan_range<expr.arity>([&](auto i, auto&& traces)
                    {
                        return find_next_expression_to_trace(
                            std::forward<decltype(traces)>(traces),
                            expr.get(i)
                        );
                    }, std::forward<decltype(traces)>(traces));
                };

                if constexpr (hmpc::cacheable_expression<E>)
                {
                    return hmpc::iter::scan_range<expr.arity>([&](auto i, auto&& traces)
                    {
                        return trace_expression(
                            std::forward<decltype(traces)>(traces),
                            hmpc::detail::type_list<hmpc::detail::type_triple<E, decltype(E::access(i)), decltype(i)>>{},
                            expr.get(i)
                        );
                    }, traverse_children());
                }
                else
                {
                    return traverse_children();
                }
            }
            else
            {
                return std::forward<decltype(traces)>(traces);
            }
        }

        template<hmpc::expression E, hmpc::expression... Exprs>
        constexpr decltype(auto) trace_expressions(auto&& traces, E expr, Exprs... exprs) HMPC_HOST_NOEXCEPT
        {
            if constexpr (sizeof...(exprs) == 0)
            {
                return find_next_expression_to_trace(std::forward<decltype(traces)>(traces), expr);
            }
            else
            {
                return trace_expressions(
                    find_next_expression_to_trace(std::forward<decltype(traces)>(traces), expr),
                    exprs...
                );
            }
        }

        template<typename... Triples>
        constexpr bool is_not_multiple_access(hmpc::detail::type_list<Triples...> const&) noexcept
        {
            return ((hmpc::access::traits::access_pattern_v<typename Triples::second_type> != hmpc::access::pattern::multiple) and ...);
        }

        template<hmpc::expression E, bool IsTopLevel>
        constexpr auto cache_expression(auto&& cache, auto const& traces, hmpc::bool_constant<IsTopLevel>, E expr) HMPC_HOST_NOEXCEPT
        {
            auto traverse_children = [&]() -> decltype(auto)
            {
                if constexpr (expr.arity > 0)
                {
                    return hmpc::iter::scan_range<expr.arity>([&](auto i, auto&& cache) -> decltype(auto)
                    {
                        return cache_expression(
                            std::forward<decltype(cache)>(cache),
                            traces,
                            hmpc::constants::no,
                            expr.get(i)
                        );
                    }, std::forward<decltype(cache)>(cache));
                }
                else
                {
                    return std::forward<decltype(cache)>(cache);
                }
            };

            static_assert(hmpc::detail::implies(IsTopLevel, hmpc::cacheable_expression<E>));

            if constexpr (hmpc::cacheable_expression<E>)
            {
                if constexpr (IsTopLevel)
                {
                    // TODO: insert top level expr as soon as possible; NOT when they appear first as top level expr to cache
                    return traverse_children().insert(expr);
                }
                else if constexpr (auto traces_for_expr = traces.get_or(hmpc::detail::tag_of<E>, hmpc::detail::type_set{}); (not hmpc::complex_expression<E>) and (traces_for_expr.size == 1) and is_not_multiple_access(traces_for_expr.get(hmpc::constants::zero)))
                {
                    return traverse_children();
                }
                else
                {
                    return traverse_children().insert(expr);
                }
            }
            else
            {
                return traverse_children();
            }
        }

        template<hmpc::expression E, hmpc::expression... Exprs>
        constexpr decltype(auto) cache_expressions(auto&& cache, auto const& traces, E expr, Exprs... exprs) HMPC_HOST_NOEXCEPT
        {
            if constexpr (sizeof...(exprs) == 0)
            {
                return cache_expression(
                    std::forward<decltype(cache)>(cache),
                    traces,
                    hmpc::constants::yes,
                    expr
                );
            }
            else
            {
                return cache_expressions(
                    cache_expression(
                        std::forward<decltype(cache)>(cache),
                        traces,
                        hmpc::constants::yes,
                        expr
                    ),
                    traces,
                    exprs...
                );
            }
        }
    }

    template<hmpc::expression... Exprs>
    constexpr decltype(auto) generate_execution_cache(Exprs... exprs) HMPC_NOEXCEPT
    {
        auto&& traces = detail::trace_expressions(hmpc::detail::type_map{}, cache(exprs)...);
        return detail::cache_expressions(hmpc::detail::type_set{}, traces, cache(exprs)...);
    }
}
