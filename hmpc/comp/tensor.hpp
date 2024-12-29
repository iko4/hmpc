#pragma once

#include <hmpc/index.hpp>

#include <sycl/buffer.hpp>

namespace hmpc::comp
{
    template<hmpc::value T, hmpc::size... Dimensions>
    struct tensor
    {
    public:
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using limb_type = hmpc::traits::limb_type_t<value_type>;
        using shape_type = hmpc::shape<Dimensions...>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;
        static constexpr hmpc::size limb_size = hmpc::traits::limb_size_v<value_type>;

    private:
        sycl::buffer<limb_type, 2> HMPC_PRIVATE_MEMBER(buffer);
        shape_type HMPC_PRIVATE_MEMBER(shape);

    public:
        constexpr tensor(shape_type shape)
            : HMPC_PRIVATE_MEMBER(buffer){sycl::range{limb_size, hmpc::element_shape<value_type>(shape).size()}}
            , HMPC_PRIVATE_MEMBER(shape)(shape)
        {
        }

        template<typename Self>
        constexpr auto&& get(this Self&& self) noexcept
        {
            return std::forward<Self>(self).HMPC_PRIVATE_MEMBER(buffer);
        }

        constexpr shape_type const& shape() const noexcept
        {
            return HMPC_PRIVATE_MEMBER(shape);
        }

        constexpr element_shape_type element_shape() const HMPC_NOEXCEPT
        {
            return hmpc::element_shape<value_type>(HMPC_PRIVATE_MEMBER(shape));
        }
    };

    template<hmpc::value T>
        requires (hmpc::scalar<T> or (hmpc::vector<T> and hmpc::traits::vector_size_v<T> == 1))
    struct tensor<T>
    {
    public:
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using limb_type = hmpc::traits::limb_type_t<value_type>;
        using shape_type = hmpc::shape<>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;
        static constexpr hmpc::size limb_size = hmpc::traits::limb_size_v<value_type>;

    private:
        sycl::buffer<element_type, 1> HMPC_PRIVATE_MEMBER(buffer);

    public:
        constexpr tensor(shape_type)
            : HMPC_PRIVATE_MEMBER(buffer){sycl::range{1}}
        {
        }

        constexpr sycl::buffer<element_type, 1>& get() noexcept
        {
            return HMPC_PRIVATE_MEMBER(buffer);
        }

        constexpr shape_type shape() const noexcept
        {
            return {};
        }

        constexpr element_shape_type element_shape() const HMPC_NOEXCEPT
        {
            static_assert(element_shape_type::dynamic_rank == 0);
            return {};
        }
    };

    template<typename T, hmpc::size... Dimensions>
    constexpr auto make_tensor(shape<Dimensions...> shape)
    {
        return tensor<T, Dimensions...>(shape);
    }
}
