#pragma once

#include <hmpc/config.hpp>
#include <hmpc/detail/type_list.hpp>
#include <hmpc/detail/type_map_element.hpp>
#include <hmpc/detail/type_tag.hpp>
#include <hmpc/iter/next.hpp>
#include <hmpc/iter/unpack_range.hpp>

namespace hmpc::detail
{
    template<typename Indices, typename Keys, typename Values>
    struct type_map_base;

    template<hmpc::size... Is, typename... Ks, typename... Vs>
    struct type_map_base<std::integer_sequence<hmpc::size, Is...>, hmpc::detail::type_list<Ks...>, hmpc::detail::type_list<Vs...>> : public indexed_type_map_element<Is, Ks, Vs>...
    {
        using indexed_type_map_element<Is, Ks, Vs>::get...;
        using indexed_type_map_element<Is, Ks, Vs>::key...;
        using indexed_type_map_element<Is, Ks, Vs>::index_of...;

        template<hmpc::size I>
        constexpr void key(hmpc::size_constant<I> = {}) const noexcept
        {
        }
    };

    template<typename Keys = hmpc::detail::type_list<>, typename Values = hmpc::detail::type_list<>>
    struct type_map : public type_map_base<std::make_integer_sequence<hmpc::size, Keys::size>, Keys, Values>
    {
    };

    template<typename... Ks, typename... Vs>
    struct type_map<hmpc::detail::type_list<Ks...>, hmpc::detail::type_list<Vs...>> : public type_map_base<std::make_integer_sequence<hmpc::size, sizeof...(Ks)>, hmpc::detail::type_list<Ks...>, hmpc::detail::type_list<Vs...>>
    {
        static constexpr auto size = hmpc::size_constant_of<sizeof...(Ks)>;

        template<typename K>
        static constexpr bool contains(hmpc::detail::type_tag<K> = {}) noexcept
        {
            return (std::same_as<K, Ks> or ...);
        }

        template<typename Self, typename K, typename V>
        constexpr auto&& get_or(this Self&& self, hmpc::detail::type_tag<K> key, V&& value) noexcept
        {
            if constexpr (std::remove_cvref_t<Self>::contains(key))
            {
                return std::forward<Self>(self).get(key);
            }
            else
            {
                return std::forward<V>(value);
            }
        }

        template<typename K, typename V>
        constexpr auto insert(hmpc::detail::type_tag<K> key, V&& value) && HMPC_NOEXCEPT
        {
            static_assert(not contains(key));
            using T = std::remove_cvref_t<V>;
            return type_map<hmpc::detail::type_list<Ks..., K>, hmpc::detail::type_list<Vs..., T>>{std::move(*this).get(hmpc::detail::tag_of<Ks>)..., std::forward<V>(value)};
        }

        template<typename K, typename V>
        constexpr decltype(auto) insert_or_assign(hmpc::detail::type_tag<K> key, V&& value) && HMPC_NOEXCEPT
        {
            using T = std::remove_cvref_t<V>;
            using This = std::remove_cvref_t<decltype(*this)>;
            if constexpr (This::contains(key))
            {
                constexpr auto index = This::index_of(key);
                if constexpr (std::same_as<std::remove_cvref_t<decltype(this->get(key))>, T>)
                {
                    this->get(key) = std::forward<V>(value);
                    return std::move(*this);
                }
                else
                {
                    return hmpc::iter::unpack(hmpc::range(index), [&](auto... i) -> decltype(auto)
                    {
                        return hmpc::iter::unpack(hmpc::range(hmpc::iter::next(index), This::size), [&](auto... j) -> decltype(auto)
                        {
                            constexpr auto values = hmpc::detail::type_list<Vs...>{};

                            return type_map<hmpc::detail::type_list<Ks...>, hmpc::detail::type_list<typename decltype(values.get(i))::type..., T, typename decltype(values.get(j))::type...>>{std::move(*this).get(i)..., std::forward<V>(value), std::move(*this).get(j)...};
                        });
                    });
                }
            }
            else
            {
                return type_map<hmpc::detail::type_list<Ks..., K>, hmpc::detail::type_list<Vs..., T>>{std::move(*this).get(hmpc::detail::tag_of<Ks>)..., std::forward<V>(value)};
            }
        }
    };
}
