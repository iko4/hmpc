#include "catch_helpers.hpp"

#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/queue.hpp>
#include <hmpc/comp/tensor.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/ints/numeric.hpp>
#include <hmpc/ints/poly.hpp>

TEST_CASE("Integer polynomials", "[ints][poly]")
{
    using small = hmpc::ints::sbigint<2>;
    using large = hmpc::ints::ubigint<20>;
    using limb = small::limb_type;

    constexpr hmpc::size N = 1 << 14;
    constexpr hmpc::size M = 2;
    using small_R = hmpc::ints::poly<small, N>;
    using large_R = hmpc::ints::poly<large, N>;

    constexpr auto shape = hmpc::shape{M};

    auto x_tensor = hmpc::comp::make_tensor<small_R>(shape);
    auto y_tensor = hmpc::comp::make_tensor<large_R>(shape);
    {
        hmpc::comp::host_accessor x(x_tensor, hmpc::access::discard_write);
        hmpc::comp::host_accessor y(y_tensor, hmpc::access::discard_write);

        for (hmpc::size n = 0; n < M; ++n)
        {
            for (hmpc::size i = 0; i < N; ++i)
            {
                x[hmpc::index{n, i}] = (n % 2) ? -small{1} : small{1};
                y[hmpc::index{n, i}] = hmpc::ints::num::bit_copy<large>(hmpc::core::size_limb_span<limb>{i});
            }
        }
    }

    using namespace hmpc::expr::operators;

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    auto x = hmpc::expr::tensor(x_tensor);
    auto y = hmpc::expr::tensor(y_tensor);

    auto [z_tensor, equal_tensor] = queue(x + y, x == y);

    using sum_R = decltype(z_tensor)::value_type;
    using sum = sum_R::element_type;

    REQUIRE(hmpc::is_signed(sum::signedness));
    REQUIRE(sum::bit_size == 21);

    REQUIRE(z_tensor.shape().rank == shape.rank);
    REQUIRE(z_tensor.shape().size() == shape.size());
    REQUIRE(equal_tensor.shape().rank == shape.rank);
    REQUIRE(equal_tensor.shape().size() == shape.size());

    {
        hmpc::comp::host_accessor z(z_tensor, hmpc::access::read);
        hmpc::comp::host_accessor equal(equal_tensor, hmpc::access::read);

        for (hmpc::size n = 0; n < M; ++n)
        {
            for (hmpc::size i = 0; i < N; ++i)
            {
                CHECK(equal[hmpc::index{n, i}] == hmpc::bit{(i == 1) and ((n % 2) == 0)});
                if ((i == 0) and (n % 2))
                {
                    CHECK(z[hmpc::index{n, i}] == -hmpc::ints::one<limb>);
                }
                else
                {
                    CHECK(z[hmpc::index{n, i}] == hmpc::ints::num::bit_copy<large>(hmpc::core::size_limb_span<limb>{i + (n % 2 ? -1 : 1)}));
                }
            }
        }
    }
}
