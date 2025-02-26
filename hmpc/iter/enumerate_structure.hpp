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
        constexpr auto enumerate_element(T const& tuple, F const& f, hmpc::size_constant<I> i, hmpc::size_constant<ElementIndex> element_index) HMPC_NOEXCEPT
        {
            if constexpr (element_index < T::size)
            {
                auto const& element = tuple.get(element_index);
                if constexpr (hmpc::structure<decltype(element)>)
                {
                    auto j = enumerate_element(element, f, i, hmpc::constants::zero);
                    return enumerate_element(tuple, f, j, hmpc::iter::next(element_index));
                }
                else
                {
                    f(i, element);
                    return enumerate_element(tuple, f, hmpc::iter::next(i), hmpc::iter::next(element_index));
                }
            }
            else
            {
                return i;
            }
        }
    }

    template<hmpc::structure T, typename F>
    constexpr void enumerate(T const& tuple, F const& f) HMPC_NOEXCEPT
    {
        detail::enumerate_element(tuple, f, hmpc::constants::zero, hmpc::constants::zero);
    }
}
