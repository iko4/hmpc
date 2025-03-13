#pragma once

#include <hmpc/access.hpp>
#include <hmpc/comp/tensor.hpp>

#include <tuple>

namespace hmpc
{
    template<typename Accessor>
    concept accessor = requires(Accessor const& accessor, hmpc::traits::dynamic_index_t<typename Accessor::element_shape_type> index)
    {
        typename Accessor::element_type;
        accessor[index];
        accessor[hmpc::size{}];
    };
}

namespace hmpc::comp
{
    namespace detail
    {
        enum class target
        {
            device,
            host,
            host_task,
        };

        template<typename T, hmpc::size Dimension, typename Access, target Target>
        struct accessor_type
        {
            static constexpr sycl::access::mode access_mode = []()
            {
                if constexpr (hmpc::access::is_discard<Access> and hmpc::access::is_write_only<Access>)
                {
                    return sycl::access::mode::discard_write;
                }
                else if constexpr (hmpc::access::is_discard<Access> and hmpc::access::is_read_write<Access>)
                {
                    return sycl::access::mode::discard_read_write;
                }
                else if constexpr (not hmpc::access::is_discard<Access> and hmpc::access::is_read_only<Access>)
                {
                    return sycl::access::mode::read;
                }
                else if constexpr (not hmpc::access::is_discard<Access> and hmpc::access::is_write_only<Access>)
                {
                    return sycl::access::mode::write;
                }
                else if constexpr (not hmpc::access::is_discard<Access> and hmpc::access::is_read_write<Access>)
                {
                    return sycl::access::mode::read_write;
                }
                else
                {
                    HMPC_COMPILETIME_ASSERT(false);
                }
            }();

            using type = decltype([]()
            {
                if constexpr (Target == target::host)
                {
                    return sycl::host_accessor<T, Dimension, access_mode>();
                }
                else if constexpr (Target == target::host_task)
                {
                    return sycl::accessor<T, Dimension, access_mode, sycl::target::host_task>();
                }
                else
                {
                    static_assert(Target == target::device);
                    return sycl::accessor<T, Dimension, access_mode>();
                }
            }());
        };
        template<typename T, hmpc::size Dimension, typename Access, target Target>
        using accessor_type_t = accessor_type<T, Dimension, Access, Target>::type;
    }

    template<hmpc::scalar T, typename... Reference>
    struct accessor_reference
    {
    public:
        using value_type = T;
        using limb_type = hmpc::traits::limb_type_t<value_type>;
        static constexpr auto limb_size = hmpc::traits::limb_size<value_type>{};

    private:
        static_assert(sizeof...(Reference) == limb_size);
        std::tuple<Reference...> references;

    public:
        constexpr accessor_reference(Reference... references)
            : references(references...)
        {
        }

        constexpr operator value_type() const
        {
            if constexpr (hmpc::traits::has_limbs_v<value_type>)
            {
                value_type result;
                hmpc::iter::for_each(hmpc::range(limb_size), [&](auto i)
                {
                    result.data[i] = std::get<i>(references);
                });
                return result;
            }
            else
            {
                static_assert(limb_size == 1);
                return std::get<0>(references);
            }
        }

        template<typename Other>
        explicit constexpr operator Other() const
        {
            return static_cast<Other>(static_cast<value_type>(*this));
        }

        constexpr accessor_reference& operator=(value_type const& value)
        {
            if constexpr (hmpc::traits::has_limbs_v<value_type>)
            {
                hmpc::iter::for_each(hmpc::range(limb_size), [&](auto i)
                {
                    std::get<i>(references) = value.data[i];
                });
            }
            else
            {
                static_assert(limb_size == 1);
                std::get<0>(references) = value;
            }
            return *this;
        }
    };

    template<hmpc::value T, typename Access, typename Shape, detail::target Target>
    struct base_accessor
    {
    public:
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using access_type = Access;
        using limb_type = hmpc::traits::limb_type_t<value_type>;
        using shape_type = Shape;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;
        static constexpr auto limb_size = hmpc::traits::limb_size<value_type>{};

    private:
        using accessor_type = detail::accessor_type_t<limb_type, 2, access_type, Target>;
        accessor_type HMPC_PRIVATE_MEMBER(accessor);
        element_shape_type HMPC_PRIVATE_MEMBER(element_shape);

    public:
        template<typename Tensor, typename Handler>
            requires (Target != detail::target::host)
        constexpr base_accessor(Tensor& tensor, Handler& handler, access_type)
            : HMPC_PRIVATE_MEMBER(accessor){tensor.get(), handler}
            , HMPC_PRIVATE_MEMBER(element_shape)(tensor.element_shape())
        {
        }

        template<typename Tensor>
            requires (Target == detail::target::host)
        constexpr base_accessor(Tensor& tensor, access_type)
            : HMPC_PRIVATE_MEMBER(accessor){tensor.get()}
            , HMPC_PRIVATE_MEMBER(element_shape)(tensor.element_shape())
        {
        }

        constexpr accessor_type& get() noexcept
        {
            return HMPC_PRIVATE_MEMBER(accessor);
        }

        constexpr hmpc::access::traits::pointer_t<limb_type, access_type> data() const noexcept
        {
            return HMPC_PRIVATE_MEMBER(accessor).get_pointer();
        }

        constexpr auto element_shape() const HMPC_NOEXCEPT
        {
            return HMPC_PRIVATE_MEMBER(element_shape);
        }

        constexpr auto operator[](hmpc::size i) const HMPC_NOEXCEPT
        {
            return hmpc::iter::unpack(hmpc::range(limb_size), [&](auto... j)
            {
                return accessor_reference<element_type, decltype(HMPC_PRIVATE_MEMBER(accessor)[sycl::id<2>{j, i}])...>{HMPC_PRIVATE_MEMBER(accessor)[sycl::id<2>{j, i}]...};
            });
        }

        constexpr auto operator[](hmpc::mdindex_for<element_shape_type> auto const& index) const HMPC_NOEXCEPT
        {
            auto i = hmpc::to_linear_index(index, HMPC_PRIVATE_MEMBER(element_shape));
            return (*this)[i];
        }
    };
    template<hmpc::value T, typename Access, detail::target Target>
        requires (hmpc::scalar<T> or (hmpc::vector<T> and hmpc::traits::vector_size_v<T> == 1))
    struct base_accessor<T, Access, hmpc::shape<>, Target>
    {
    public:
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using access_type = Access;
        using limb_type = hmpc::traits::limb_type_t<value_type>;
        using shape_type = hmpc::shape<>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;
        static constexpr hmpc::size limb_size = hmpc::traits::limb_size_v<value_type>;

    private:
        using accessor_type = detail::accessor_type_t<element_type, 1, access_type, Target>;
        accessor_type HMPC_PRIVATE_MEMBER(accessor);

    public:
        template<typename Tensor, typename Handler>
            requires (Target != detail::target::host)
        constexpr base_accessor(Tensor& tensor, Handler& handler, access_type)
            : HMPC_PRIVATE_MEMBER(accessor){tensor.get(), handler}
        {
        }

        template<typename Tensor>
            requires (Target == detail::target::host)
        constexpr base_accessor(Tensor& tensor, access_type)
            : HMPC_PRIVATE_MEMBER(accessor){tensor.get()}
        {
        }

        constexpr accessor_type& get() noexcept
        {
            return HMPC_PRIVATE_MEMBER(accessor);
        }

        constexpr hmpc::access::traits::pointer_t<limb_type, access_type> data() const noexcept
        {
            static_assert(sizeof(element_type) == sizeof(limb_type) * limb_size);
            using pointer_type = hmpc::access::traits::pointer_t<limb_type, access_type>;
            return reinterpret_cast<pointer_type>(HMPC_PRIVATE_MEMBER(accessor).get_pointer());
        }

        constexpr base_accessor& operator=(element_type const& value) HMPC_NOEXCEPT
            requires hmpc::access::is_write<access_type>
        {
            HMPC_PRIVATE_MEMBER(accessor)[0] = value;
            return *this;
        }

        constexpr element_shape_type element_shape() const noexcept
        {
            return {};
        }

        constexpr auto&& operator[](hmpc::index_for<element_shape_type> auto const&) const HMPC_NOEXCEPT
        {
            // TODO: assert that index is 0
            return HMPC_PRIVATE_MEMBER(accessor)[0];
        }

        constexpr operator element_type() const HMPC_NOEXCEPT
            requires hmpc::access::is_read<access_type>
        {
            return HMPC_PRIVATE_MEMBER(accessor)[0];
        }
    };

    template<hmpc::value T, typename Access, typename Shape>
    struct device_accessor : public base_accessor<T, Access, Shape, detail::target::device>
    {
        using base_accessor<T, Access, Shape, detail::target::device>::base_accessor;
    };
    template<typename Tensor, typename Handler, typename Access>
    device_accessor(Tensor& tensor, Handler& handler, Access) -> device_accessor<typename Tensor::value_type, Access, typename Tensor::shape_type>;

    template<hmpc::value T, typename Access, typename Shape>
    struct host_accessor : public base_accessor<T, Access, Shape, detail::target::host>
    {
        using base_accessor<T, Access, Shape, detail::target::host>::base_accessor;
    };
    template<typename Tensor, typename Access>
    host_accessor(Tensor& tensor, Access) -> host_accessor<typename Tensor::value_type, Access, typename Tensor::shape_type>;

    template<hmpc::value T, typename Access, typename Shape>
    struct host_task_accessor : public base_accessor<T, Access, Shape, detail::target::host_task>
    {
        using base_accessor<T, Access, Shape, detail::target::host_task>::base_accessor;
    };
    template<typename Tensor, typename Handler, typename Access>
    host_task_accessor(Tensor& tensor, Handler& handler, Access) -> host_task_accessor<typename Tensor::value_type, Access, typename Tensor::shape_type>;
}
