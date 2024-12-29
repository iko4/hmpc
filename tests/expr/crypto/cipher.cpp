#include "catch_helpers.hpp"

#include <hmpc/expr/crypto/cipher.hpp>
#include <hmpc/ints/bigint.hpp>
#include <hmpc/comp/queue.hpp>
#include <hmpc/expr/tensor.hpp>

TEST_CASE("Ciphers", "[expr][crypto][cipher]")
{
    using uint = hmpc::ints::ubigint<127>;
    using limb = uint::limb_type;

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    auto shape = hmpc::shape{4, 2};

    auto x = hmpc::comp::make_tensor<uint>(shape);
    {
        hmpc::comp::host_accessor x_access(x, hmpc::access::discard_write);
        for (hmpc::size i = 0; i < shape.size(); ++i)
        {
            x_access[i] = hmpc::ints::num::bit_copy<uint>(hmpc::core::size_limb_span<limb>{i});
        }
    }

    hmpc::core::limb_array<hmpc::default_random_engine::key_size, hmpc::default_random_engine::value_type> key = {42};
    hmpc::core::limb_array<hmpc::default_random_engine::nonce_size, hmpc::default_random_engine::value_type> nonce = {1};

    auto y = queue(
        hmpc::expr::crypto::enc(
            hmpc::expr::crypto::cipher(
                key.span(hmpc::access::read),
                nonce.span(hmpc::access::read)
            ),
            hmpc::expr::tensor(x)
        )
    );

    REQUIRE(y.shape().rank == 3);
    REQUIRE(y.shape().get(hmpc::constants::zero) == 4);
    REQUIRE(y.shape().get(hmpc::constants::one) == 2);
    REQUIRE(y.shape().get(hmpc::constants::two) == 4);

    auto x_again = queue(
        hmpc::expr::crypto::dec<uint>(
            hmpc::expr::crypto::cipher(
                key.span(hmpc::access::read),
                nonce.span(hmpc::access::read)
            ),
            hmpc::expr::tensor(y)
        )
    );

    REQUIRE(x_again.shape().rank == 2);
    REQUIRE(x_again.shape().get(hmpc::constants::zero) == 4);
    REQUIRE(x_again.shape().get(hmpc::constants::one) == 2);

    {
        hmpc::comp::host_accessor x_access(x_again, hmpc::access::read);
        for (hmpc::size i = 0; i < shape.size(); ++i)
        {
            CHECK(x_access[i] == hmpc::ints::num::bit_copy<uint>(hmpc::core::size_limb_span<limb>{i}));
        }
    }
}
