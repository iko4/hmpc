#pragma once

#include <hmpc/config.hpp>
#include <hmpc/range.hpp>

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

    template<typename Index, typename Difference, Index Count, typename F, typename T>
    constexpr decltype(auto) scan(hmpc::range<Index, Difference, 0, Count, 1>, F const& f, T&& initial) HMPC_NOEXCEPT
    {
        using index_type = Index;
        return [&]<index_type... I>(std::integer_sequence<index_type, I...>) -> decltype(auto)
        {
            return (std::forward<T>(initial) | ... | detail::scan_state{f, hmpc::constant_of<I>});
        }(std::make_integer_sequence<index_type, Count>{});
    }

    template<typename Index, typename Difference, Index Start, Index End, typename F, typename T>
    constexpr decltype(auto) scan(hmpc::range<Index, Difference, Start, End, 1>, F const& f, T&& initial) HMPC_NOEXCEPT
    {
        using index_type = Index;
        static_assert(Start <= End);
        using difference_type = Difference;
        return [&]<difference_type... I>(std::integer_sequence<difference_type, I...>) -> decltype(auto)
        {
            return (std::forward<T>(initial) | ... | detail::scan_state{f, hmpc::constant_of<index_type{Start + I}>});
        }(std::make_integer_sequence<difference_type, End - Start>{});
    }
}
