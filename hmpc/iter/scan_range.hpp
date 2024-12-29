#pragma once

#include <hmpc/config.hpp>

namespace hmpc::iter
{
    namespace detail
    {
        template<typename F, typename T>
        struct scan_state
        {
            F const& function;
            T value;

            constexpr scan_state(F const& function, T value) HMPC_NOEXCEPT
                : function(function)
                , value(value)
            {
            }

            template<typename V>
            friend constexpr decltype(auto) operator|(V&& value, scan_state const& sequencer) HMPC_NOEXCEPT
            {
                return sequencer.function(sequencer.value, std::forward<V>(value));
            }
        };
    }

    template<auto V, typename F, typename T>
    constexpr decltype(auto) scan_range(F const& f, T&& initial) HMPC_NOEXCEPT
    {
        using index_type = decltype(V);
        return [&]<index_type... I>(std::integer_sequence<index_type, I...>) -> decltype(auto)
        {
            return (std::forward<T>(initial) | ... | detail::scan_state{f, hmpc::constant_of<I>});
        }(std::make_integer_sequence<index_type, V>{});
    }

    template<auto Begin, auto End, typename F, typename T>
    constexpr decltype(auto) scan_range(F const& f, T&& initial) HMPC_NOEXCEPT
    {
        using index_type = decltype(Begin);
        static_assert(std::same_as<index_type, decltype(End)>);
        static_assert(Begin <= End);
        using difference_type = decltype(End - Begin);
        return [&]<difference_type... I>(std::integer_sequence<difference_type, I...>) -> decltype(auto)
        {
            return (std::forward<T>(initial) | ... | detail::scan_state{f, hmpc::constant_of<index_type{Begin + I}>});
        }(std::make_integer_sequence<difference_type, End - Begin>{});
    }
}
