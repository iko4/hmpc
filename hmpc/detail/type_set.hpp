#pragma once

#include <hmpc/detail/tuple.hpp>

namespace hmpc::detail
{
    template<typename... Ts>
    struct type_set : public tuple<Ts...>
    {
        template<typename Self, typename Value>
        constexpr decltype(auto) insert(this Self&& self, Value&& value) HMPC_NOEXCEPT
        {
            // TODO: (?) use std::forward_like
            using T = std::remove_cvref_t<Value>;
            if constexpr (std::remove_cvref_t<Self>::contains(hmpc::detail::tag_of<T>))
            {
                return std::forward<Self>(self);
            }
            else
            {
                return type_set<Ts..., T>{std::forward<Self>(self).get(hmpc::detail::tag_of<Ts>)..., std::forward<Value>(value)};
            }
        }
    };
}
