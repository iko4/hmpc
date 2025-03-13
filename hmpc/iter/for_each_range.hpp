#pragma once

#include <hmpc/config.hpp>
#include <hmpc/range.hpp>

namespace hmpc::iter
{
    template<typename Index, typename Difference, Index Count, typename F>
    constexpr decltype(auto) for_each(hmpc::range<Index, Difference, 0, Count, 1>, F const& f) HMPC_NOEXCEPT
    {
        using index_type = Index;
        return [&f]<index_type... I>(std::integer_sequence<index_type, I...>) -> decltype(auto)
        {
            return (f(hmpc::constant_of<I>), ...);
        }(std::make_integer_sequence<index_type, Count>{});
    }

    template<typename Index, typename Difference, Index Start, Index End, typename F>
    constexpr decltype(auto) for_each(hmpc::range<Index, Difference, Start, End, 1>, F const& f) HMPC_NOEXCEPT
    {
        using index_type = Index;
        static_assert(Start <= End);
        using difference_type = Difference;
        return [&f]<difference_type... I>(std::integer_sequence<difference_type, I...>) -> decltype(auto)
        {
            return (f(hmpc::constant_of<index_type{Start + I}>), ...);
        }(std::make_integer_sequence<difference_type, End - Start>{});
    }
}
