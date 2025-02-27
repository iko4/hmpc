#pragma once

#include <hmpc/config.hpp>
#include <hmpc/constants.hpp>
#include <hmpc/iter/next.hpp>
#include <hmpc/typing/structure.hpp>

namespace hmpc::iter
{
    namespace detail
    {
        template<hmpc::structure T, typename F, hmpc::size I, hmpc::size ElementIndex>
        constexpr auto enumerate_element(T& tuple, F const& f, hmpc::size_constant<I> i, hmpc::size_constant<ElementIndex> element_index) HMPC_NOEXCEPT
        {
            if constexpr (element_index < T::size)
            {
                decltype(auto) element = tuple.get(element_index);
                if constexpr (hmpc::structure<decltype(element)>)
                {
                    auto j = enumerate_element(std::forward<decltype(element)>(element), f, i, hmpc::constants::zero);
                    return enumerate_element(tuple, f, j, hmpc::iter::next(element_index));
                }
                else
                {
                    f(i, std::forward<decltype(element)>(element));
                    return enumerate_element(tuple, f, hmpc::iter::next(i), hmpc::iter::next(element_index));
                }
            }
            else
            {
                return i;
            }
        }

        template<typename T, typename... Rest, typename F, hmpc::size I>
        constexpr auto enumerate_elements(F const& f, hmpc::size_constant<I> i, T& first, Rest&... rest) HMPC_NOEXCEPT
        {
            if constexpr (hmpc::structure<T>)
            {
                auto j = enumerate_element(first, f, i, hmpc::constants::zero);
                if constexpr (sizeof...(Rest) > 0)
                {
                    enumerate_elements(f, j, rest...);
                }
            }
            else
            {
                f(i, first);
                if constexpr (sizeof...(Rest) > 0)
                {
                    return enumerate_elements(f, hmpc::iter::next(i), rest...);
                }
            }
        }
    }

    template<hmpc::structure T, typename F>
    constexpr void enumerate(T& tuple, F const& f) HMPC_NOEXCEPT
    {
        detail::enumerate_element(tuple, f, hmpc::constants::zero, hmpc::constants::zero);
    }

    template<typename... Ts, typename F>
    constexpr void enumerate(F const& f, Ts&... values) HMPC_NOEXCEPT
    {
        detail::enumerate_elements(f, hmpc::constants::zero, values...);
    }
}
