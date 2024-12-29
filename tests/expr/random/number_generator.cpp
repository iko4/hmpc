#include "catch_helpers.hpp"

#include <hmpc/comp/queue.hpp>
#include <hmpc/expr/random/uniform_from_number_generator.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/ints/literals.hpp>
#include <hmpc/ints/poly_mod.hpp>

TEST_CASE("Uniform from number generator", "[expr][random]")
{
    using namespace hmpc::expr::operators;
    using namespace hmpc::ints::literals;

    constexpr auto p = 0x8822'd806'2332'0001_int; // 9809640459238244353
    constexpr auto N = hmpc::size{1} << 16;
    constexpr auto M = hmpc::size{11};

    using rng = hmpc::default_random_engine;

    using Rp = hmpc::ints::poly_mod<p, N, hmpc::ints::coefficient_representation>;
    using ntt_Rp = hmpc::ints::traits::number_theoretic_transform_type_t<Rp>;
    using mod_p = Rp::element_type;
    using plaintext = ntt_Rp;
    using limb = plaintext::limb_type;
    using uint = mod_p::unsigned_type;

    auto x_tensor = hmpc::comp::make_tensor<plaintext>(hmpc::shape{M});
    {
        hmpc::comp::host_accessor x(x_tensor, hmpc::access::discard_write);

        for (hmpc::size i = 0; i < N * M; ++i)
        {
            x[i] = mod_p{uint{static_cast<limb>(i)}};
        }
    }

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    hmpc::core::limb_array<rng::key_size, rng::value_type> key = {42};
    auto r = hmpc::expr::random::uniform<plaintext>(
        hmpc::expr::random::number_generator(
            key.span(hmpc::access::read),
            hmpc::index{1},
            hmpc::shape{2}
        ),
        hmpc::shape{M}
    );

    auto r_tensor = queue(r);

    auto [m_tensor, r_tensor_copy] = queue(r + r + hmpc::expr::tensor(r_tensor) + hmpc::expr::tensor(x_tensor), r);
    {
        hmpc::comp::host_accessor r(r_tensor, hmpc::access::read);
        hmpc::comp::host_accessor r_copy(r_tensor_copy, hmpc::access::read);
        hmpc::comp::host_accessor m(m_tensor, hmpc::access::read);

        for (hmpc::size i = 0; i < N * M; ++i)
        {
            CHECK(m[i] - r[i] - r[i] == r[i] + mod_p{uint{static_cast<limb>(i)}});
            CHECK(r[i] == r_copy[i]);
        }
    }
}
