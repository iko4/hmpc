#pragma once

#include <hmpc/comp/tensor.hpp>

namespace hmpc::comp::crypto::lhe
{
    template<typename Poly, hmpc::size... Dimensions>
    struct ciphertext
    {
        hmpc::comp::tensor<Poly, Dimensions...> c0;
        hmpc::comp::tensor<Poly, Dimensions...> c1;

        using is_structure = void;
        static constexpr hmpc::size size = 2;

        constexpr auto& get(hmpc::size_constant<0>) const
        {
            return c0;
        }

        constexpr auto& get(hmpc::size_constant<1>) const
        {
            return c1;
        }
    };
}
