#pragma once

#include <hmpc/config.hpp>
#include <hmpc/detail/type_map_element.hpp>
#include <hmpc/detail/type_tag.hpp>

namespace hmpc::detail
{
    template<hmpc::size Index, typename T>
    using indexed_type_set_element = indexed_type_map_element<Index, T, T>;

    template<typename Indices, typename... Ts>
    struct type_set_base;

    template<hmpc::size... Is, typename... Ts>
    struct type_set_base<std::integer_sequence<hmpc::size, Is...>, Ts...> : public indexed_type_set_element<Is, Ts>...
    {
        using indexed_type_set_element<Is, Ts>::get...;
        using indexed_type_set_element<Is, Ts>::index_of...;

        static constexpr hmpc::size size = sizeof...(Ts);

        template<typename T>
        static constexpr bool contains(hmpc::detail::type_tag<T> = {}) noexcept
        {
            return (std::same_as<T, Ts> or ...);
        }

        template<hmpc::size I>
        constexpr void get(hmpc::size_constant<I> = {}) const noexcept
        {
        }
    };

    template<typename... Ts>
    struct tuple : public type_set_base<std::make_integer_sequence<hmpc::size, sizeof...(Ts)>, Ts...>
    {
    };
}
