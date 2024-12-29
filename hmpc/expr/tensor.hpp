#pragma once

#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/tensor.hpp>
#include <hmpc/value.hpp>

namespace hmpc::expr
{
    template<hmpc::value T, auto Tag = []{}, hmpc::size... Dimensions>
    struct tensor_expression
    {
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using shape_type = hmpc::shape<Dimensions...>;
        using element_shape_type = hmpc::traits::element_shape_t<value_type, shape_type>;

        static constexpr hmpc::size arity = 0;

        hmpc::comp::tensor<value_type, Dimensions...>* tensor;

        constexpr tensor_expression(hmpc::comp::tensor<value_type, Dimensions...>& tensor) HMPC_NOEXCEPT
            : tensor(std::addressof(tensor))
        {
        }

        constexpr decltype(auto) shape() const HMPC_NOEXCEPT
        {
            HMPC_HOST_ASSERT(tensor != nullptr);
            return tensor->shape();
        }

        constexpr auto state(auto& handler) const HMPC_NOEXCEPT
        {
            HMPC_HOST_ASSERT(tensor != nullptr);
            return hmpc::comp::device_accessor(*tensor, handler, hmpc::access::read);
        }

        static constexpr element_type operator()(hmpc::accessor auto const& accessor, hmpc::index_for<element_shape_type> auto const& index, auto const&) HMPC_NOEXCEPT
        {
            return accessor[index];
        }
    };

    template<auto Tag = []{}, hmpc::value T, hmpc::size... Dimensions>
    constexpr auto tensor(hmpc::comp::tensor<T, Dimensions...>& b) HMPC_NOEXCEPT
    {
        return tensor_expression<T, Tag, Dimensions...>{b};
    }
}
