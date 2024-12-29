#include "catch_helpers.hpp"

#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/tensor.hpp>
#include <hmpc/comp/queue.hpp>
#include <hmpc/expr/bit_monomial.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/core/size_limb_span.hpp>

TEST_CASE("Monomial multiplication", "[ints][poly]")
{
    constexpr hmpc::size N = 1 << 3;
    constexpr auto shape = hmpc::shape{1, 2 * N + 1};

    using integer = hmpc::ints::sbigint<N>;
    using limb = integer::limb_type;
    using R = hmpc::ints::poly<integer, N>;

    auto index_tensor = hmpc::comp::make_tensor<std::optional<hmpc::size>>(shape);
    {
        hmpc::comp::host_accessor indices(index_tensor, hmpc::access::discard_write);

        for (hmpc::size i = 0; i < shape.get(hmpc::constants::zero); ++i)
        {
            for (hmpc::size j = 0; j < shape.get(hmpc::constants::one); ++j)
            {
                auto index = hmpc::index{i, j};
                if (j < 2 * N)
                {
                    indices[index] = j;
                }
                else
                {
                    indices[index] = std::nullopt;
                }
            }
        }
    }
    auto x_tensor = hmpc::comp::make_tensor<R>(shape);
    {
        hmpc::comp::host_accessor x(x_tensor, hmpc::access::discard_write);

        for (hmpc::size i = 0; i < shape.get(hmpc::constants::zero); ++i)
        {
            for (hmpc::size j = 0; j < shape.get(hmpc::constants::one); ++j)
            {
                for (hmpc::size k = 0; k < N; ++k)
                {
                    auto index = hmpc::index{i, j, k};
                    auto value = hmpc::ints::num::bit_copy<integer>(hmpc::core::size_limb_span<limb>{k + 1});
                    x[index] = value;
                }
            }
        }
    }

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    auto index = hmpc::expr::tensor(index_tensor);
    auto monomial = hmpc::expr::bit_monomial<N>(index);
    auto x = hmpc::expr::tensor(x_tensor);

    using namespace hmpc::expr::operators;

    std::array<integer, 8> numbers =
    {
        integer{1},
        integer{2},
        integer{3},
        integer{4},
        integer{5},
        integer{6},
        integer{7},
        integer{8},
    };

    auto y_tensor = queue(x * monomial);
    {
        hmpc::comp::host_accessor y(y_tensor, hmpc::access::read);
        CHECK(y[hmpc::index<0, 0, 0>{}] == numbers[0]);
        CHECK(y[hmpc::index<0, 0, 1>{}] == numbers[1]);
        CHECK(y[hmpc::index<0, 0, 2>{}] == numbers[2]);
        CHECK(y[hmpc::index<0, 0, 3>{}] == numbers[3]);
        CHECK(y[hmpc::index<0, 0, 4>{}] == numbers[4]);
        CHECK(y[hmpc::index<0, 0, 5>{}] == numbers[5]);
        CHECK(y[hmpc::index<0, 0, 6>{}] == numbers[6]);
        CHECK(y[hmpc::index<0, 0, 7>{}] == numbers[7]);

        CHECK(y[hmpc::index<0, 1, 0>{}] == -numbers[7]);
        CHECK(y[hmpc::index<0, 1, 1>{}] == numbers[0]);
        CHECK(y[hmpc::index<0, 1, 2>{}] == numbers[1]);
        CHECK(y[hmpc::index<0, 1, 3>{}] == numbers[2]);
        CHECK(y[hmpc::index<0, 1, 4>{}] == numbers[3]);
        CHECK(y[hmpc::index<0, 1, 5>{}] == numbers[4]);
        CHECK(y[hmpc::index<0, 1, 6>{}] == numbers[5]);
        CHECK(y[hmpc::index<0, 1, 7>{}] == numbers[6]);

        CHECK(y[hmpc::index<0, 2, 0>{}] == -numbers[6]);
        CHECK(y[hmpc::index<0, 2, 1>{}] == -numbers[7]);
        CHECK(y[hmpc::index<0, 2, 2>{}] == numbers[0]);
        CHECK(y[hmpc::index<0, 2, 3>{}] == numbers[1]);
        CHECK(y[hmpc::index<0, 2, 4>{}] == numbers[2]);
        CHECK(y[hmpc::index<0, 2, 5>{}] == numbers[3]);
        CHECK(y[hmpc::index<0, 2, 6>{}] == numbers[4]);
        CHECK(y[hmpc::index<0, 2, 7>{}] == numbers[5]);

        CHECK(y[hmpc::index<0, 3, 0>{}] == -numbers[5]);
        CHECK(y[hmpc::index<0, 3, 1>{}] == -numbers[6]);
        CHECK(y[hmpc::index<0, 3, 2>{}] == -numbers[7]);
        CHECK(y[hmpc::index<0, 3, 3>{}] == numbers[0]);
        CHECK(y[hmpc::index<0, 3, 4>{}] == numbers[1]);
        CHECK(y[hmpc::index<0, 3, 5>{}] == numbers[2]);
        CHECK(y[hmpc::index<0, 3, 6>{}] == numbers[3]);
        CHECK(y[hmpc::index<0, 3, 7>{}] == numbers[4]);

        CHECK(y[hmpc::index<0, 4, 0>{}] == -numbers[4]);
        CHECK(y[hmpc::index<0, 4, 1>{}] == -numbers[5]);
        CHECK(y[hmpc::index<0, 4, 2>{}] == -numbers[6]);
        CHECK(y[hmpc::index<0, 4, 3>{}] == -numbers[7]);
        CHECK(y[hmpc::index<0, 4, 4>{}] == numbers[0]);
        CHECK(y[hmpc::index<0, 4, 5>{}] == numbers[1]);
        CHECK(y[hmpc::index<0, 4, 6>{}] == numbers[2]);
        CHECK(y[hmpc::index<0, 4, 7>{}] == numbers[3]);

        CHECK(y[hmpc::index<0, 5, 0>{}] == -numbers[3]);
        CHECK(y[hmpc::index<0, 5, 1>{}] == -numbers[4]);
        CHECK(y[hmpc::index<0, 5, 2>{}] == -numbers[5]);
        CHECK(y[hmpc::index<0, 5, 3>{}] == -numbers[6]);
        CHECK(y[hmpc::index<0, 5, 4>{}] == -numbers[7]);
        CHECK(y[hmpc::index<0, 5, 5>{}] == numbers[0]);
        CHECK(y[hmpc::index<0, 5, 6>{}] == numbers[1]);
        CHECK(y[hmpc::index<0, 5, 7>{}] == numbers[2]);

        CHECK(y[hmpc::index<0, 6, 0>{}] == -numbers[2]);
        CHECK(y[hmpc::index<0, 6, 1>{}] == -numbers[3]);
        CHECK(y[hmpc::index<0, 6, 2>{}] == -numbers[4]);
        CHECK(y[hmpc::index<0, 6, 3>{}] == -numbers[5]);
        CHECK(y[hmpc::index<0, 6, 4>{}] == -numbers[6]);
        CHECK(y[hmpc::index<0, 6, 5>{}] == -numbers[7]);
        CHECK(y[hmpc::index<0, 6, 6>{}] == numbers[0]);
        CHECK(y[hmpc::index<0, 6, 7>{}] == numbers[1]);

        CHECK(y[hmpc::index<0, 7, 0>{}] == -numbers[1]);
        CHECK(y[hmpc::index<0, 7, 1>{}] == -numbers[2]);
        CHECK(y[hmpc::index<0, 7, 2>{}] == -numbers[3]);
        CHECK(y[hmpc::index<0, 7, 3>{}] == -numbers[4]);
        CHECK(y[hmpc::index<0, 7, 4>{}] == -numbers[5]);
        CHECK(y[hmpc::index<0, 7, 5>{}] == -numbers[6]);
        CHECK(y[hmpc::index<0, 7, 6>{}] == -numbers[7]);
        CHECK(y[hmpc::index<0, 7, 7>{}] == numbers[0]);

        CHECK(y[hmpc::index<0, 8, 0>{}] == -numbers[0]);
        CHECK(y[hmpc::index<0, 8, 1>{}] == -numbers[1]);
        CHECK(y[hmpc::index<0, 8, 2>{}] == -numbers[2]);
        CHECK(y[hmpc::index<0, 8, 3>{}] == -numbers[3]);
        CHECK(y[hmpc::index<0, 8, 4>{}] == -numbers[4]);
        CHECK(y[hmpc::index<0, 8, 5>{}] == -numbers[5]);
        CHECK(y[hmpc::index<0, 8, 6>{}] == -numbers[6]);
        CHECK(y[hmpc::index<0, 8, 7>{}] == -numbers[7]);

        CHECK(y[hmpc::index<0, 9, 0>{}] == numbers[7]);
        CHECK(y[hmpc::index<0, 9, 1>{}] == -numbers[0]);
        CHECK(y[hmpc::index<0, 9, 2>{}] == -numbers[1]);
        CHECK(y[hmpc::index<0, 9, 3>{}] == -numbers[2]);
        CHECK(y[hmpc::index<0, 9, 4>{}] == -numbers[3]);
        CHECK(y[hmpc::index<0, 9, 5>{}] == -numbers[4]);
        CHECK(y[hmpc::index<0, 9, 6>{}] == -numbers[5]);
        CHECK(y[hmpc::index<0, 9, 7>{}] == -numbers[6]);

        CHECK(y[hmpc::index<0, 10, 0>{}] == numbers[6]);
        CHECK(y[hmpc::index<0, 10, 1>{}] == numbers[7]);
        CHECK(y[hmpc::index<0, 10, 2>{}] == -numbers[0]);
        CHECK(y[hmpc::index<0, 10, 3>{}] == -numbers[1]);
        CHECK(y[hmpc::index<0, 10, 4>{}] == -numbers[2]);
        CHECK(y[hmpc::index<0, 10, 5>{}] == -numbers[3]);
        CHECK(y[hmpc::index<0, 10, 6>{}] == -numbers[4]);
        CHECK(y[hmpc::index<0, 10, 7>{}] == -numbers[5]);

        CHECK(y[hmpc::index<0, 11, 0>{}] == numbers[5]);
        CHECK(y[hmpc::index<0, 11, 1>{}] == numbers[6]);
        CHECK(y[hmpc::index<0, 11, 2>{}] == numbers[7]);
        CHECK(y[hmpc::index<0, 11, 3>{}] == -numbers[0]);
        CHECK(y[hmpc::index<0, 11, 4>{}] == -numbers[1]);
        CHECK(y[hmpc::index<0, 11, 5>{}] == -numbers[2]);
        CHECK(y[hmpc::index<0, 11, 6>{}] == -numbers[3]);
        CHECK(y[hmpc::index<0, 11, 7>{}] == -numbers[4]);

        CHECK(y[hmpc::index<0, 12, 0>{}] == numbers[4]);
        CHECK(y[hmpc::index<0, 12, 1>{}] == numbers[5]);
        CHECK(y[hmpc::index<0, 12, 2>{}] == numbers[6]);
        CHECK(y[hmpc::index<0, 12, 3>{}] == numbers[7]);
        CHECK(y[hmpc::index<0, 12, 4>{}] == -numbers[0]);
        CHECK(y[hmpc::index<0, 12, 5>{}] == -numbers[1]);
        CHECK(y[hmpc::index<0, 12, 6>{}] == -numbers[2]);
        CHECK(y[hmpc::index<0, 12, 7>{}] == -numbers[3]);

        CHECK(y[hmpc::index<0, 13, 0>{}] == numbers[3]);
        CHECK(y[hmpc::index<0, 13, 1>{}] == numbers[4]);
        CHECK(y[hmpc::index<0, 13, 2>{}] == numbers[5]);
        CHECK(y[hmpc::index<0, 13, 3>{}] == numbers[6]);
        CHECK(y[hmpc::index<0, 13, 4>{}] == numbers[7]);
        CHECK(y[hmpc::index<0, 13, 5>{}] == -numbers[0]);
        CHECK(y[hmpc::index<0, 13, 6>{}] == -numbers[1]);
        CHECK(y[hmpc::index<0, 13, 7>{}] == -numbers[2]);

        CHECK(y[hmpc::index<0, 14, 0>{}] == numbers[2]);
        CHECK(y[hmpc::index<0, 14, 1>{}] == numbers[3]);
        CHECK(y[hmpc::index<0, 14, 2>{}] == numbers[4]);
        CHECK(y[hmpc::index<0, 14, 3>{}] == numbers[5]);
        CHECK(y[hmpc::index<0, 14, 4>{}] == numbers[6]);
        CHECK(y[hmpc::index<0, 14, 5>{}] == numbers[7]);
        CHECK(y[hmpc::index<0, 14, 6>{}] == -numbers[0]);
        CHECK(y[hmpc::index<0, 14, 7>{}] == -numbers[1]);

        CHECK(y[hmpc::index<0, 15, 0>{}] == numbers[1]);
        CHECK(y[hmpc::index<0, 15, 1>{}] == numbers[2]);
        CHECK(y[hmpc::index<0, 15, 2>{}] == numbers[3]);
        CHECK(y[hmpc::index<0, 15, 3>{}] == numbers[4]);
        CHECK(y[hmpc::index<0, 15, 4>{}] == numbers[5]);
        CHECK(y[hmpc::index<0, 15, 5>{}] == numbers[6]);
        CHECK(y[hmpc::index<0, 15, 6>{}] == numbers[7]);
        CHECK(y[hmpc::index<0, 15, 7>{}] == -numbers[0]);

        CHECK(y[hmpc::index<0, 16, 0>{}] == hmpc::ints::zero<limb>);
        CHECK(y[hmpc::index<0, 16, 1>{}] == hmpc::ints::zero<limb>);
        CHECK(y[hmpc::index<0, 16, 2>{}] == hmpc::ints::zero<limb>);
        CHECK(y[hmpc::index<0, 16, 3>{}] == hmpc::ints::zero<limb>);
        CHECK(y[hmpc::index<0, 16, 4>{}] == hmpc::ints::zero<limb>);
        CHECK(y[hmpc::index<0, 16, 5>{}] == hmpc::ints::zero<limb>);
        CHECK(y[hmpc::index<0, 16, 6>{}] == hmpc::ints::zero<limb>);
        CHECK(y[hmpc::index<0, 16, 7>{}] == hmpc::ints::zero<limb>);
    }
}
