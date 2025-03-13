#pragma once

#include <hmpc/config.hpp>
#include <hmpc/constant.hpp>

namespace hmpc::core
{
    template<hmpc::size Index, hmpc::size Extent>
    struct indexed_mdsize_element
    {
        constexpr indexed_mdsize_element() noexcept = default;
        constexpr indexed_mdsize_element(hmpc::size_constant<Extent>) noexcept
        {
        }

        template<typename Self>
        constexpr auto get(this Self&&, hmpc::size_constant<Index>) noexcept
        {
            return hmpc::size_constant_of<Extent>;
        }

        static constexpr hmpc::size extent(hmpc::size_constant<Index>) noexcept
        {
            return Extent;
        }
    };

    template<hmpc::size Index>
    struct indexed_mdsize_element<Index, hmpc::dynamic_extent>
    {
        hmpc::size value;

        constexpr indexed_mdsize_element() noexcept = default;
        constexpr indexed_mdsize_element(hmpc::size value) noexcept
            : value(value)
        {
        }

        template<typename Self>
        constexpr auto&& get(this Self&& self, hmpc::size_constant<Index>) noexcept
        {
            return std::forward<Self>(self).indexed_mdsize_element<Index, hmpc::dynamic_extent>::value;
        }

        static constexpr hmpc::size extent(hmpc::size_constant<Index>) noexcept
        {
            return hmpc::dynamic_extent;
        }
    };

    template<typename Indices, hmpc::size... Extents>
    struct mdsize_base;

    template<hmpc::size... Is, hmpc::size... Extents>
    struct mdsize_base<std::integer_sequence<hmpc::size, Is...>, Extents...> : public indexed_mdsize_element<Is, Extents>...
    {
        using indexed_mdsize_element<Is, Extents>::get...;
        using indexed_mdsize_element<Is, Extents>::extent...;

        template<hmpc::size I>
        constexpr auto get() const noexcept
        {
            return get(hmpc::size_constant_of<I>);
        }
    };

    template<hmpc::size... Extents>
    struct mdsize : public mdsize_base<std::make_integer_sequence<hmpc::size, sizeof...(Extents)>, Extents...>
    {
        static constexpr auto rank = hmpc::size_constant_of<sizeof...(Extents)>;

        static constexpr auto static_rank = hmpc::size_constant_of<(hmpc::size{0} + ... + hmpc::size{Extents == hmpc::dynamic_extent ? 0 : 1})>;

        static constexpr auto dynamic_rank = hmpc::size_constant_of<(hmpc::size{0} + ... + hmpc::size{Extents == hmpc::dynamic_extent ? 1 : 0})>;

        static_assert(rank == static_rank + dynamic_rank);

        static constexpr auto is_static = hmpc::bool_constant_of<(rank == static_rank)>;

        using index_type = hmpc::size;
        using size_type = hmpc::size;
        using rank_type = hmpc::size;
    };
    template<typename... Args>
    mdsize(Args&&...) -> mdsize<hmpc::traits::value_or_v<std::remove_cvref_t<Args>, hmpc::dynamic_extent>...>;
}
