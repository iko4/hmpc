#pragma once

#include <hmpc/iter/scan_range.hpp>

namespace hmpc::iter
{
    template<auto V, typename F, typename T>
    constexpr decltype(auto) scan_reverse_range(F const& f, T&& initial) HMPC_NOEXCEPT
    {
        using index_type = decltype(V);
        return [&]<index_type... I>(std::integer_sequence<index_type, I...>) -> decltype(auto)
        {
            return (std::forward<T>(initial) | ... | detail::scan_state{f, hmpc::constant_of<index_type{V - I - 1}>});
        }(std::make_integer_sequence<index_type, V>{});
    }
}
