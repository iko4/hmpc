#include "catch_helpers.hpp"

#include <hmpc/comp/queue.hpp>
#include <hmpc/expr/crypto/lhe/dec.hpp>
#include <hmpc/expr/crypto/lhe/enc.hpp>
#include <hmpc/expr/value.hpp>
#include <hmpc/ints/literals.hpp>
#include <hmpc/ints/poly_mod.hpp>

#include <ranges>

TEST_CASE("Linear BGV", "[expr][crypto][lhe]")
{
    using namespace hmpc::ints::literals;

    constexpr auto p = 0x8822'd806'2332'0001_int;                                                                     // 9809640459238244353
    constexpr auto q = 0x59'1f5b'834c'0d96'1f67'343b'cc89'02bd'eda2'771f'5430'6ff1'5116'2ff8'd2b4'0f41'94dc'0001_int; // 676310504550516370745208338938566342426856908484397554505023779011987369401721290753
    constexpr auto N = hmpc::size{1} << 16;

    using Rq = hmpc::ints::poly_mod<q, N, hmpc::ints::coefficient_representation>;
    using ntt_Rq = hmpc::ints::traits::number_theoretic_transform_type_t<Rq>;
    using mod_q = Rq::element_type;
    using limb = mod_q::limb_type;

    using Rp = hmpc::ints::poly_mod<p, N, hmpc::ints::coefficient_representation>;
    using ntt_Rp = hmpc::ints::traits::number_theoretic_transform_type_t<Rp>;
    using mod_p = Rp::element_type;

    using plaintext = ntt_Rp;

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    auto shape = hmpc::shape{4, 2};

    auto a = hmpc::comp::make_tensor<ntt_Rq>(hmpc::shape{});
    for (hmpc::comp::host_accessor access(a, hmpc::access::discard_write); hmpc::size i : std::views::iota(hmpc::size{}, N))
    {
        access[i] = hmpc::ints::num::bit_copy<mod_q>(hmpc::core::size_limb_span<limb>(i));
    }
    auto b = a;
    auto s = hmpc::comp::make_tensor<ntt_Rq>(hmpc::shape{});
    for (hmpc::comp::host_accessor access(s, hmpc::access::discard_write); hmpc::size i : std::views::iota(hmpc::size{}, N))
    {
        access[i] = hmpc::ints::integer_traits<mod_q>::one;
    }

    auto k = hmpc::comp::crypto::lhe::key{a, b};

    auto x = hmpc::comp::make_tensor<plaintext>(shape);
    for (hmpc::comp::host_accessor access(x, hmpc::access::discard_write); hmpc::size i : std::views::iota(hmpc::size{}, 4 * 2 * N))
    {
        access[i] = hmpc::ints::num::bit_copy<mod_p>(hmpc::core::size_limb_span<limb>(i));
    }

    auto r = hmpc::expr::crypto::lhe::randomness<Rq>(shape);

    auto c = queue(hmpc::expr::crypto::lhe::enc(hmpc::expr::crypto::lhe::key(k), hmpc::expr::tensor(x), r));

    auto c_expr = hmpc::expr::crypto::lhe::ciphertext(c);

    auto [d, e] = queue(c_expr + c_expr, hmpc::expr::value(mod_q{hmpc::ints::ubigint<2>{2}}) * c_expr);

    // TODO: test decryption and homomorphic properties

    auto y = queue(hmpc::expr::crypto::lhe::dec<plaintext>(hmpc::expr::tensor(s), c_expr));
    REQUIRE(y.shape().rank == 2);
    REQUIRE(y.shape().get(hmpc::constants::zero) == 4);
    REQUIRE(y.shape().get(hmpc::constants::one) == 2);
    for (hmpc::comp::host_accessor access(y, hmpc::access::discard_write); hmpc::size i : std::views::iota(hmpc::size{}, 4 * 2 * N))
    {
        CHECK(access[i] == hmpc::ints::num::bit_copy<mod_p>(hmpc::core::size_limb_span<limb>(i)));
    }
}
