#pragma once

#include <hmpc/comp/tensor.hpp>

namespace hmpc::comp::crypto::lhe
{
    template<typename Poly>
    struct key
    {
        hmpc::comp::tensor<Poly> a;
        hmpc::comp::tensor<Poly> b;
    };
}
