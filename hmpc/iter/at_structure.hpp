#pragma once

#include <hmpc/iter/next.hpp>
#include <hmpc/iter/prev.hpp>
#include <hmpc/typing/structure.hpp>

namespace hmpc::iter
{
    template<hmpc::structure T, hmpc::size I, hmpc::size ElementIndex = 0>
    constexpr decltype(auto) at(T&& structure, hmpc::size_constant<I> i, hmpc::size_constant<ElementIndex> element_index = {}) HMPC_NOEXCEPT
    {
        using element_type = hmpc::typing::traits::structure_element_t<T, ElementIndex>;
        if constexpr (hmpc::structure<element_type>)
        {
            constexpr auto fields = hmpc::typing::traits::structure_fields<element_type>{};
            if constexpr (fields <= i)
            {
                return at(structure, prev(i, fields), next(element_index));
            }
            else
            {
                return at(structure.get(element_index), i);
            }
        }
        else
        {
            if constexpr (i == 0)
            {
                return structure.get(element_index);
            }
            else
            {
                static_assert(i > 0);
                return at(structure, prev(i), next(element_index));
            }
        }
    }

    template<hmpc::size I, typename T, typename... Rest>
    constexpr decltype(auto) at(hmpc::size_constant<I> i, T&& first, Rest&&... rest) HMPC_NOEXCEPT
    {
        constexpr auto fields = hmpc::typing::traits::structure_fields<T>{};
        if constexpr (i < fields)
        {
            if constexpr (hmpc::structure<T>)
            {
                return at(first, i);
            }
            else
            {
                static_assert(i == 0);
                return first;
            }
        }
        else
        {
            return at(prev(i, fields), rest...);
        }
    }
}
