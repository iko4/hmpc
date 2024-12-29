#pragma once

#include <hmpc/shape.hpp>
#include <hmpc/detail/constant_list.hpp>
#include <hmpc/iter/for_range.hpp>
#include <hmpc/iter/scan_range.hpp>
#include <hmpc/iter/scan_reverse_range.hpp>

#include <tuple>

namespace hmpc
{
    template<hmpc::size... Extents>
    struct index : public hmpc::core::mdsize<Extents...>
    {
        static constexpr bool has_placeholder = ((Extents == hmpc::placeholder_extent) or ...);
    };
    template<typename... Args>
    index(Args&&...) -> index<hmpc::traits::value_or_v<std::remove_cvref_t<Args>, hmpc::dynamic_extent>...>;

    namespace traits
    {
        template<typename Shape>
        struct dynamic_index;

        template<hmpc::size... Extents>
        struct dynamic_index<shape<Extents...>>
        {
            using type = index<(Extents != hmpc::placeholder_extent ? hmpc::dynamic_extent : hmpc::placeholder_extent)...>;
        };

        template<typename Shape>
        using dynamic_index_t = dynamic_index<Shape>::type;
    }

    /// Similarly to `common_shape`, we can index an rank-0 shape with any index.
    /// Otherwise, the rank has to match.
    template<typename Index, typename Shape>
        requires (Shape::rank == 0 or Index::rank == Shape::rank)
    constexpr auto to_linear_index(Index index, Shape shape) HMPC_DEVICE_NOEXCEPT
    {
        if constexpr (Shape::rank == 0)
        {
            return hmpc::constants::zero;
        }
        else
        {
            static_assert(Index::rank == Shape::rank);

            return hmpc::iter::scan_range<shape.rank>([&](auto i, auto linear_index)
            {
                if constexpr (shape.extent(i) == hmpc::placeholder_extent)
                {
                    return linear_index;
                }
                else
                {
                    auto value = index.get(i);
                    auto extent = shape.get(i);
                    if constexpr (hmpc::is_constant<decltype(value)> and hmpc::is_constant<decltype(extent)>)
                    {
                        static_assert(value.value >= 0);
                        static_assert(value.value < extent.value);
                    }
                    else
                    {
                        HMPC_DEVICE_ASSERT(value >= 0);
                        HMPC_DEVICE_ASSERT(value < extent);
                    }
                    return hmpc::core::add(
                        hmpc::core::multiply(linear_index, extent),
                        value
                    );
                }
            }, hmpc::constants::zero);
        }
    }

    template<typename Shape>
    constexpr auto from_linear_index(hmpc::size size, Shape shape) HMPC_DEVICE_NOEXCEPT
    {
        HMPC_DEVICE_ASSERT(size >= 0);
        HMPC_DEVICE_ASSERT(size < shape.size());
        using index_type = hmpc::traits::dynamic_index_t<Shape>;
        if constexpr (Shape::rank == 0)
        {
            // empty shape implies empty index
            static_assert(index_type::rank == 0);
            return index_type{};
        }
        else
        {
            index_type index; // TODO: implement version for constant `size` parameter
            hmpc::iter::scan_reverse_range<Shape::rank>([&](auto i, auto size)
            {
                if constexpr (shape.extent(i) == hmpc::placeholder_extent)
                {
                    return size;
                }
                else
                {
                    index.get(i) = size % shape.get(i);
                    return size / shape.get(i);
                }
            }, size);
            return index;
        }
    }

    template<hmpc::maybe_signed_size_constant Dim, hmpc::size... Extents>
    constexpr auto unsqueeze(index<Extents...> const& value, Dim dim, hmpc::force_tag) noexcept
    {
        using index_type = index<Extents...>;
        constexpr hmpc::size rank = index_type::rank;
        constexpr hmpc::size pos = [&]()
        {
            if constexpr (dim < 0)
            {
                static_assert(-dim <= rank);
                return rank + dim;
            }
            else
            {
                static_assert(dim < rank);
                return dim;
            }
        }();

        static_assert(0 <= pos);
        static_assert(pos < rank);

        return hmpc::iter::for_packed_range<pos>([&](auto... i)
        {
            return hmpc::iter::for_packed_range<pos + 1, rank>([&](auto... j)
            {
                return index{value.get(i)..., value.get(j)...};
            });
        });
    }

    template<hmpc::value T, hmpc::maybe_signed_size_constant Dim, hmpc::size... Extents>
    constexpr auto unsqueeze_for_element_index(index<Extents...> const& value, Dim dim, hmpc::force_tag) noexcept
    {
        using index_type = index<Extents...>;
        constexpr hmpc::size rank = index_type::rank - (hmpc::vector<T> ? 1 : 0);
        constexpr hmpc::size pos = [&]()
        {
            if constexpr (dim < 0)
            {
                static_assert(-dim <= rank);
                return rank + dim;
            }
            else
            {
                static_assert(dim < rank);
                return dim;
            }
        }();

        static_assert(0 <= pos);
        static_assert(pos < rank);

        return hmpc::iter::for_packed_range<pos>([&](auto... i)
        {
            return hmpc::iter::for_packed_range<pos + 1, rank>([&](auto... j)
            {
                if constexpr (hmpc::vector<T>)
                {
                    constexpr auto element_index = hmpc::size_constant_of<rank>;
                    return index{value.get(i)..., value.get(j)..., value.get(element_index)};
                }
                else
                {
                    return index{value.get(i)..., value.get(j)...};
                }
            });
        });
    }
}

namespace hmpc
{
    template<typename Index, typename Shape>
    concept linear_index_for = hmpc::maybe_constant_of<Index, typename Shape::index_type>;

    template<typename Index, typename Shape>
    concept mdindex_for = requires(Index index, Shape shape)
    {
        hmpc::to_linear_index(index, shape);
    };

    template<typename Index, typename Shape>
    concept index_for = linear_index_for<Index, Shape> or mdindex_for<Index, Shape>;
}

template<hmpc::size... Extents>
struct std::tuple_size<hmpc::index<Extents...>> : std::integral_constant<std::size_t, sizeof...(Extents)>
{
};

template<std::size_t I, hmpc::size... Extents>
struct std::tuple_element<I, hmpc::index<Extents...>>
{
    using index_type = hmpc::index<Extents...>;
    static_assert(I >= 0);
    static_assert(I < index_type::rank);
    using type = std::remove_cvref_t<decltype(std::declval<index_type>().template get<I>())>;
};
