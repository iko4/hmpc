#pragma once

#include <hmpc/config.hpp>
#include <hmpc/constant.hpp>

namespace hmpc::detail
{
    template<hmpc::size Index, typename T, T Value>
    struct indexed_constant_list_element
    {
        static constexpr hmpc::size index = Index;

        constexpr indexed_constant_list_element() noexcept = default;
        constexpr indexed_constant_list_element(hmpc::constant<T, Value>) noexcept
        {
        }

        static constexpr hmpc::constant<T, Value> get(hmpc::size_constant<Index>) noexcept
        {
            return {};
        }

        static constexpr hmpc::size_constant<index> index_of(hmpc::constant<T, Value>) noexcept
        {
            return {};
        }
    };

    template<typename Indices, typename T, T... Values>
    struct constant_list_base;

    template<hmpc::size... Is, typename T, T... Values>
    struct constant_list_base<std::integer_sequence<hmpc::size, Is...>, T, Values...> : public indexed_constant_list_element<Is, T, Values>...
    {
        using indexed_constant_list_element<Is, T, Values>::get...;
        using indexed_constant_list_element<Is, T, Values>::index_of...;
    };

    template<typename T, T... Values>
    struct constant_list : public constant_list_base<std::make_integer_sequence<hmpc::size, sizeof...(Values)>, T, Values...>
    {
        static constexpr auto size = hmpc::size_constant_of<sizeof...(Values)>;

        template<T Value>
        static constexpr bool contains(hmpc::constant<T, Value> = {}) noexcept
        {
            return ((Values == Value) or ...);
        }

        template<T Value>
        static constexpr auto append(hmpc::constant<T, Value> = {}) noexcept
        {
            return constant_list<T, Values..., Value>{};
        }

        template<T... OtherValues>
        static constexpr auto append(constant_list<T, OtherValues...> = {}) noexcept
        {
            return constant_list<T, Values..., OtherValues...>{};
        }
    };
}
