#pragma once

#include <hmpc/comp/tensor.hpp>

namespace hmpc::comp::crypto::lhe
{
    template<typename Poly, hmpc::size... Dimensions>
    struct ciphertext
    {
        hmpc::comp::tensor<Poly, Dimensions...> c0;
        hmpc::comp::tensor<Poly, Dimensions...> c1;
    };
}
