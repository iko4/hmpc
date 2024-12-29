#pragma once

#include <hmpc/detail/constant_list.hpp>
#include <hmpc/core/mdsize.hpp>
#include <hmpc/core/multiply.hpp>
#include <hmpc/iter/for_range.hpp>
#include <hmpc/iter/for_packed_range.hpp>
#include <hmpc/iter/scan_range.hpp>
#include <hmpc/value.hpp>

#include <tuple>

namespace hmpc
{
    struct shapeless_tag
    {
    };
    constexpr shapeless_tag shapeless = {};


    template<hmpc::size... Extents>
    struct shape : public hmpc::core::mdsize<Extents...>
    {
        using hmpc::core::mdsize<Extents...>::rank;

        static constexpr bool has_placeholder = ((Extents == hmpc::placeholder_extent) or ...);

        constexpr auto size() const HMPC_NOEXCEPT
        {
            return hmpc::iter::scan_range<rank>([&](auto i, auto accumulated)
            {
                auto extent = this->get(i);
                if constexpr (hmpc::is_constant<decltype(extent)>)
                {
                    if constexpr (extent.value == hmpc::placeholder_extent)
                    {
                        return accumulated;
                    }
                    else
                    {
                        return hmpc::core::multiply(accumulated, extent);
                    }
                }
                else
                {
                    HMPC_DEVICE_ASSERT(extent > 0);
                    return accumulated * extent;
                }
            }, hmpc::constants::one);
        }

        static constexpr auto static_size() noexcept
        {
            if constexpr (rank == 0)
            {
                return 1;
            }
            else
            {
                hmpc::size size = 1;
                hmpc::iter::for_range<rank>([&](auto i)
                {
                    constexpr auto e = hmpc::core::mdsize<Extents...>::extent(i);
                    if constexpr(e != hmpc::dynamic_extent and e != hmpc::placeholder_extent)
                    {
                        size *= e;
                    }
                });
                return size;
            }
        }
    };
    template<typename... Args>
    shape(Args&&...) -> shape<hmpc::traits::value_or_v<std::remove_cvref_t<Args>, hmpc::dynamic_extent>...>;

    template<hmpc::maybe_constant_of<hmpc::size> Left, hmpc::maybe_constant_of<hmpc::size> Right>
    constexpr auto common_extent(Left left, Right right) HMPC_NOEXCEPT
    {
        if constexpr (hmpc::is_constant<Left> and hmpc::is_constant<Right>)
        {
            static_assert(left.value != hmpc::dynamic_extent);
            static_assert(right.value != hmpc::dynamic_extent);
            if constexpr (left.value == hmpc::placeholder_extent)
            {
                return right;
            }
            else if constexpr (right.value == hmpc::placeholder_extent)
            {
                return left;
            }
            else
            {
                static_assert(left.value == right.value);
                return left;
            }
        }
        else if constexpr (hmpc::is_constant<Left>)
        {
            static_assert(left.value == hmpc::placeholder_extent);
            return right;
        }
        else if constexpr (hmpc::is_constant<Right>)
        {
            static_assert(right.value == hmpc::placeholder_extent);
            return left;
        }
        else
        {
            HMPC_ASSERT(left == right);
            return left;
        }
    }

    template<hmpc::size... LeftExtents, hmpc::size... RightExtents>
    constexpr auto common_shape(shape<LeftExtents...> const& left, shape<RightExtents...> const& right) HMPC_NOEXCEPT
    {
        using left_shape = shape<LeftExtents...>;
        using right_shape = shape<RightExtents...>;
        if constexpr (left_shape::rank == 0)
        {
            return right;
        }
        else if constexpr (right_shape::rank == 0)
        {
            return left;
        }
        else
        {
            static_assert(left_shape::rank == right_shape::rank);

            return hmpc::iter::for_packed_range<left_shape::rank>([&](auto... i)
            {
                return shape{common_extent(left.get(i), right.get(i))...};
            });
        }
    }

    template<hmpc::maybe_signed_size_constant Dim, hmpc::maybe_constant_of<hmpc::size> Extent = hmpc::size_constant<hmpc::placeholder_extent>, hmpc::size... Extents>
    constexpr auto unsqueeze(shape<Extents...> const& value, Dim dim = {}, Extent extent = {}) noexcept
    {
        using shape_type = shape<Extents...>;
        constexpr hmpc::size pos = [&]()
        {
            if constexpr (dim < 0)
            {
                static_assert(-dim <= shape_type::rank + 1);
                return shape_type::rank + 1 + dim;
            }
            else
            {
                static_assert(dim <= shape_type::rank);
                return dim;
            }
        }();

        static_assert(0 <= pos);
        static_assert(pos <= shape_type::rank);

        return hmpc::iter::for_packed_range<pos>([&](auto... i)
        {
            return hmpc::iter::for_packed_range<pos, shape_type::rank>([&](auto... j)
            {
                return shape{value.get(i)..., extent, value.get(j)...};
            });
        });
    }

    struct force_tag
    {
    };
    constexpr force_tag force = {};

    template<hmpc::maybe_signed_size_constant Dim, hmpc::size... Extents>
    constexpr auto squeeze(shape<Extents...> const& value, Dim dim, force_tag) noexcept
    {
        using shape_type = shape<Extents...>;
        constexpr hmpc::size rank = shape_type::rank;
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
                return shape{value.get(i)..., value.get(j)...};
            });
        });
    }

    namespace traits
    {
        template<typename T, typename Shape>
        struct element_shape;

        template<hmpc::scalar T, typename Shape>
        struct element_shape<T, Shape>
        {
            using type = Shape;
        };

        template<hmpc::vector T, hmpc::size... Extents>
        struct element_shape<T, shape<Extents...>>
        {
            using type = shape<Extents..., hmpc::traits::vector_size_v<T>>;
        };

        template<typename T, typename Shape>
        using element_shape_t = element_shape<T, Shape>::type;
    }

    template<typename T, typename Shape>
    constexpr auto element_shape(Shape const& shape) HMPC_NOEXCEPT
    {
        return hmpc::iter::for_packed_range<Shape::rank>([&](auto... i) -> traits::element_shape_t<T, Shape>
        {
            if constexpr (hmpc::scalar<T>)
            {
                return shape;
            }
            else
            {
                return {shape.get(i)..., hmpc::traits::vector_size<T>{}};
            }
        });
    }
}

template<hmpc::size... Extents>
struct std::tuple_size<hmpc::shape<Extents...>> : std::integral_constant<std::size_t, sizeof...(Extents)>
{
};

template<std::size_t I, hmpc::size... Extents>
struct std::tuple_element<I, hmpc::shape<Extents...>>
{
    using shape_type = hmpc::shape<Extents...>;
    static_assert(I >= 0);
    static_assert(I < shape_type::rank);
    using type = std::remove_cvref_t<decltype(std::declval<shape_type>().template get<I>())>;
};
