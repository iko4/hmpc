#pragma once

#include <hmpc/constant.hpp>

namespace hmpc
{
    template<typename Index, typename Difference, Index Start, Index End, Difference Step>
    struct range
    {
        using index_type = Index;
        using difference_type = Difference;

        static constexpr auto start = hmpc::constant_of<Start>;
        static constexpr auto end = hmpc::constant_of<End>;
        static constexpr auto step = hmpc::constant_of<Step>;

        template<hmpc::is_constant_of<index_type> E>
        constexpr range(E) noexcept
            requires (start == 0 and E::value == End and step == 1)
        {
        }

        template<hmpc::is_constant_of<index_type> S, hmpc::is_constant_of<index_type> E>
        constexpr range(S, E) noexcept
            requires (S::value == Start and E::value == End and step == 1)
        {
        }

        template<hmpc::is_constant_of<index_type> S, hmpc::is_constant_of<index_type> E, hmpc::is_constant_of<difference_type> T>
        constexpr range(S, E, T) noexcept
            requires (S::value == Start and E::value == End and T::value == Step)
        {
        }

        template<hmpc::is_constant_of<index_type> I>
        static constexpr bool contains(I i) noexcept
        {
            if constexpr (i < start or i >= end)
            {
                return false;
            }
            else
            {
                constexpr index_type adjusted_i = i - start;
                return (adjusted_i % step) == 0;
            }
        }
    };
    template<typename T, T End>
    range(hmpc::constant<T, End>) -> range<T, decltype(End - End), 0, End, 1>;
    template<typename T, T Start, T End>
    range(hmpc::constant<T, Start>, hmpc::constant<T, End>) -> range<T, decltype(End - Start), Start, End, 1>;
    template<typename T, typename S, T Start, T End, S Step>
    range(hmpc::constant<T, Start>, hmpc::constant<T, End>, hmpc::constant<S, Step>) -> range<T, S, Start, End, Step>;
}
