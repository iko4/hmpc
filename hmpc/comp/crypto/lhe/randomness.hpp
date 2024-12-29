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
    };
}
