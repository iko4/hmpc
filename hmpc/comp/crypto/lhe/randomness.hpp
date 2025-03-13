#pragma once

#include <hmpc/comp/tensor.hpp>

namespace hmpc::comp::crypto::lhe
{
    template<typename Poly, hmpc::size... Dimensions>
    struct randomness
    {
        hmpc::comp::tensor<Poly, Dimensions...> u;
        hmpc::comp::tensor<Poly, Dimensions...> v;
        hmpc::comp::tensor<Poly, Dimensions...> w;

        using is_structure = void;
        static constexpr auto size = hmpc::size_constant_of<3>;

        template<typename Self>
        constexpr auto&& get(this Self&& self, hmpc::size_constant<0>)
        {
            return std::forward<Self>(self).u;
        }

        template<typename Self>
        constexpr auto&& get(this Self&& self, hmpc::size_constant<1>)
        {
            return std::forward<Self>(self).v;
        }


        template<typename Self>
        constexpr auto&& get(this Self&& self, hmpc::size_constant<2>)
        {
            return std::forward<Self>(self).w;
        }

        static constexpr auto from_parts(hmpc::comp::tensor<Poly, Dimensions...>&& u, hmpc::comp::tensor<Poly, Dimensions...>&& v, hmpc::comp::tensor<Poly, Dimensions...>&& w)
        {
            return randomness{std::move(u), std::move(v), std::move(w)};
        }
    };
}
