#pragma once

#include <hmpc/config.hpp>

namespace hmpc::iter
{
    template<auto V, typename F>
    constexpr decltype(auto) for_range(F const& f) HMPC_NOEXCEPT
    {
        using index_type = decltype(V);
        return [&f]<index_type... I>(std::integer_sequence<index_type, I...>) -> decltype(auto)
        {
            return (f(hmpc::constant_of<I>), ...);
        }(std::make_integer_sequence<index_type, V>{});
    }

    template<auto Begin, auto End, typename F>
    constexpr decltype(auto) for_range(F const& f) HMPC_NOEXCEPT
    {
        using index_type = decltype(Begin);
        static_assert(std::same_as<index_type, decltype(End)>);
        static_assert(Begin <= End);
        using difference_type = decltype(End - Begin);
        return [&f]<difference_type... I>(std::integer_sequence<difference_type, I...>) -> decltype(auto)
        {
            return (f(hmpc::constant_of<index_type{Begin + I}>), ...);
        }(std::make_integer_sequence<difference_type, End - Begin>{});
    }
}
