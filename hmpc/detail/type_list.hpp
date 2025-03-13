#pragma once

#include <hmpc/constant.hpp>
#include <hmpc/detail/type_tag.hpp>

namespace hmpc::detail
{
    template<hmpc::size Index, typename T>
    struct indexed_type_list_element
    {
        static constexpr hmpc::detail::type_tag<T> get(hmpc::size_constant<Index>) noexcept
        {
            return {};
        }
    };

    template<typename Indices, typename... Ts>
    struct type_list_base;

    template<hmpc::size... Is, typename... Ts>
    struct type_list_base<std::integer_sequence<hmpc::size, Is...>, Ts...> : public indexed_type_list_element<Is, Ts>...
    {
        using indexed_type_list_element<Is, Ts>::get...;
    };

    template<typename... Ts>
    struct type_list : public type_list_base<std::make_integer_sequence<hmpc::size, sizeof...(Ts)>, Ts...>
    {
        static constexpr auto size = hmpc::size_constant_of<sizeof...(Ts)>;

        template<typename T>
        static constexpr auto append(hmpc::detail::type_tag<T> = {}) noexcept
        {
            return type_list<Ts..., T>{};
        }
    };
}
