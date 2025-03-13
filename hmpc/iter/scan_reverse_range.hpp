#pragma once

#include <hmpc/iter/scan_range.hpp>

namespace hmpc::iter
{
    template<typename Index, typename Difference, Index Count, typename F, typename T>
    constexpr decltype(auto) scan_reverse(hmpc::range<Index, Difference, 0, Count, 1>, F const& f, T&& initial) HMPC_NOEXCEPT
    {
        using index_type = Index;
        return [&]<index_type... I>(std::integer_sequence<index_type, I...>) -> decltype(auto)
        {
            return (std::forward<T>(initial) | ... | detail::scan_state{f, hmpc::constant_of<index_type{Count - I - 1}>});
        }(std::make_integer_sequence<index_type, Count>{});
    }
}
