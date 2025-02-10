#include "catch_helpers.hpp"

#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/queue.hpp>
#include <hmpc/comp/tensor.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/reduce.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/ints/uint.hpp>

TEST_CASE("Reduction", "[expr][reduce]")
{
    using namespace hmpc::expr::operators;

    constexpr auto N = 10;
    using uint = hmpc::ints::uint<127>;
    using limb = uint::limb_type;

    auto x_tensor = hmpc::comp::make_tensor<uint>(hmpc::shape{N});
    auto y_tensor = hmpc::comp::make_tensor<uint>(hmpc::shape{N});

    {
        hmpc::comp::host_accessor access_x(x_tensor, hmpc::access::discard_write);
        hmpc::comp::host_accessor access_y(y_tensor, hmpc::access::discard_write);

        for (hmpc::size i = 0; i < N; ++i)
        {
            access_x[i] = uint{static_cast<limb>(3 * i + 1)};
            access_y[i] = uint{static_cast<limb>(i + 1)};
        }
    }

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    auto x = hmpc::expr::tensor(x_tensor);
    auto y = hmpc::expr::tensor(y_tensor);

    auto [all_tensor, any_tensor] = queue(
        hmpc::expr::all(x == y),
        hmpc::expr::any(x == y)
    );

    // SECTION("any[0]")
    {
        hmpc::comp::host_accessor any(any_tensor, hmpc::access::read);
        hmpc::bit value = any[hmpc::size{0}];
        CHECK(value);
    }
    // SECTION("any")
    {
        hmpc::comp::host_accessor any(any_tensor, hmpc::access::read);
        hmpc::bit value = any;
        CHECK(value);
    }

    // SECTION("all[0]")
    {
        hmpc::comp::host_accessor all(all_tensor, hmpc::access::read);
        hmpc::bit value = all[hmpc::size{0}];
        CHECK_FALSE(value);
    }
    // SECTION("all")
    {
        hmpc::comp::host_accessor all(all_tensor, hmpc::access::read);
        hmpc::bit value = all;
        CHECK_FALSE(value);
    }

    auto [all_equal_tensor, sum_tensor, product_tensor, bit_or_tensor, bit_and_tensor, bit_xor_tensor, min_tensor, max_tensor] = queue(
        hmpc::expr::all(x == x),
        hmpc::expr::sum(x),
        hmpc::expr::product(x),
        hmpc::expr::reduce(x, hmpc::reduction::bit_or),
        hmpc::expr::reduce(x, hmpc::reduction::bit_and),
        hmpc::expr::reduce(x, hmpc::reduction::bit_xor),
        hmpc::expr::min(x),
        hmpc::expr::max(x)
    );
    {
        hmpc::comp::host_accessor all(all_equal_tensor, hmpc::access::read);
        hmpc::bit value = all;
        CHECK(value == hmpc::bit{true});
    }
    {
        hmpc::comp::host_accessor sum(sum_tensor, hmpc::access::read);
        uint value = sum;
        CHECK(value == uint{145});
    }
    {
        hmpc::comp::host_accessor product(product_tensor, hmpc::access::read);
        uint value = product;
        CHECK(value == hmpc::ints::num::bit_copy<uint>(hmpc::core::size_limb_span<limb>(17'041'024'000)));
    }
    {
        hmpc::comp::host_accessor bit_or(bit_or_tensor, hmpc::access::read);
        uint value = bit_or;
        CHECK(value == uint{31});
    }
    {
        hmpc::comp::host_accessor bit_and(bit_and_tensor, hmpc::access::read);
        uint value = bit_and;
        CHECK(value == uint{0});
    }
    {
        hmpc::comp::host_accessor bit_xor(bit_xor_tensor, hmpc::access::read);
        uint value = bit_xor;
        CHECK(value == uint{21});
    }
    {
        hmpc::comp::host_accessor min(min_tensor, hmpc::access::read);
        uint value = min;
        CHECK(value == uint{1});
    }
    {
        hmpc::comp::host_accessor max(max_tensor, hmpc::access::read);
        uint value = max;
        CHECK(value == uint{28});
    }
}
