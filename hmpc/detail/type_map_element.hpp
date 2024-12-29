#pragma once

#include <hmpc/config.hpp>
#include <hmpc/constant.hpp>
#include <hmpc/detail/type_tag.hpp>

namespace hmpc::detail
{
    template<hmpc::size Index, typename Key, typename Value>
    struct indexed_type_map_element
    {
        using key_type = Key;
        using mapped_type = Value;
        using value_type = Value;

        static constexpr hmpc::size index = Index;

        value_type value;

        constexpr indexed_type_map_element() HMPC_NOEXCEPT = default;

        constexpr indexed_type_map_element(value_type const& value) HMPC_NOEXCEPT
            : value(value)
        {
        }

        constexpr indexed_type_map_element(value_type&& value) HMPC_NOEXCEPT
            : value(std::move(value))
        {
        }

        template<typename Self>
        constexpr auto&& get(this Self&& self, hmpc::detail::type_tag<key_type>) noexcept
        {
            return std::forward<Self>(self).indexed_type_map_element<Index, Key, Value>::value;
        }

        template<typename Self>
        constexpr auto&& get(this Self&& self, hmpc::size_constant<index>) noexcept
        {
            return std::forward<Self>(self).indexed_type_map_element<Index, Key, Value>::value;
        }

        static constexpr hmpc::detail::type_tag<key_type> key(hmpc::size_constant<index>) noexcept
        {
            return {};
        }

        static constexpr hmpc::size_constant<index> index_of(hmpc::detail::type_tag<key_type>) noexcept
        {
            return {};
        }
    };
}
