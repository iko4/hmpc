#include "catch_helpers.hpp"

#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/queue.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/expr/matrix_product.hpp>
#include <hmpc/ints/bigint.hpp>

TEST_CASE("Matrix product", "[expr][matmul]")
{
    using uint = hmpc::ints::ubigint<5>;

    auto x_shape = hmpc::shape{2, 2, hmpc::constants::three};
    // x[0] =
    //  0 1 2
    //  3 4 5
    //
    // x[1] =
    //  0 1 0
    //  0 0 1
    auto x_tensor = hmpc::comp::make_tensor<uint>(x_shape);
    {
        hmpc::comp::host_accessor x(x_tensor, hmpc::access::discard_write);
        x[hmpc::index{0, 0, 0}] = uint{0};
        x[hmpc::index{0, 0, 1}] = uint{1};
        x[hmpc::index{0, 0, 2}] = uint{2};

        x[hmpc::index{0, 1, 0}] = uint{3};
        x[hmpc::index{0, 1, 1}] = uint{4};
        x[hmpc::index{0, 1, 2}] = uint{5};

        x[hmpc::index{1, 0, 0}] = uint{0};
        x[hmpc::index{1, 0, 1}] = uint{1};
        x[hmpc::index{1, 0, 2}] = uint{0};

        x[hmpc::index{1, 1, 0}] = uint{0};
        x[hmpc::index{1, 1, 1}] = uint{0};
        x[hmpc::index{1, 1, 2}] = uint{1};
    }

    auto y_shape = hmpc::shape{2, hmpc::constants::three, 4};
    // y[0] =
    //   7  8  9 10
    //  11 12 13 14
    //  15 16 17 18
    //
    // y[1] =
    //  19 20 21 22
    //  23 24 25 26
    //  27 28 29 30
    auto y_tensor = hmpc::comp::make_tensor<uint>(y_shape);
    {
        hmpc::comp::host_accessor y(y_tensor, hmpc::access::discard_write);
        y[hmpc::index{0, 0, 0}] = uint{7};
        y[hmpc::index{0, 0, 1}] = uint{8};
        y[hmpc::index{0, 0, 2}] = uint{9};
        y[hmpc::index{0, 0, 3}] = uint{10};

        y[hmpc::index{0, 1, 0}] = uint{11};
        y[hmpc::index{0, 1, 1}] = uint{12};
        y[hmpc::index{0, 1, 2}] = uint{13};
        y[hmpc::index{0, 1, 3}] = uint{14};

        y[hmpc::index{0, 2, 0}] = uint{15};
        y[hmpc::index{0, 2, 1}] = uint{16};
        y[hmpc::index{0, 2, 2}] = uint{17};
        y[hmpc::index{0, 2, 3}] = uint{18};

        y[hmpc::index{1, 0, 0}] = uint{19};
        y[hmpc::index{1, 0, 1}] = uint{20};
        y[hmpc::index{1, 0, 2}] = uint{21};
        y[hmpc::index{1, 0, 3}] = uint{22};

        y[hmpc::index{1, 1, 0}] = uint{23};
        y[hmpc::index{1, 1, 1}] = uint{24};
        y[hmpc::index{1, 1, 2}] = uint{25};
        y[hmpc::index{1, 1, 3}] = uint{26};

        y[hmpc::index{1, 2, 0}] = uint{27};
        y[hmpc::index{1, 2, 1}] = uint{28};
        y[hmpc::index{1, 2, 2}] = uint{29};
        y[hmpc::index{1, 2, 3}] = uint{30};
    }

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    auto x = hmpc::expr::tensor(x_tensor);
    auto y = hmpc::expr::tensor(y_tensor);
    auto z_tensor = queue(hmpc::expr::matrix_product(x, y));
    using result_uint = decltype(z_tensor)::element_type;
    REQUIRE(result_uint::bit_size == 12);
    auto z_shape = z_tensor.shape();
    REQUIRE(z_shape.rank == 3);
    REQUIRE(z_shape.get(hmpc::constants::zero) == 2);
    REQUIRE(z_shape.get(hmpc::constants::one) == 2);
    REQUIRE(z_shape.get(hmpc::constants::two) == 4);
    {
        hmpc::comp::host_accessor z(z_tensor, hmpc::access::read);
        CHECK(z[hmpc::index{0, 0, 0}] == result_uint{41});
        CHECK(z[hmpc::index{0, 0, 1}] == result_uint{44});
        CHECK(z[hmpc::index{0, 0, 2}] == result_uint{47});
        CHECK(z[hmpc::index{0, 0, 3}] == result_uint{50});

        CHECK(z[hmpc::index{0, 1, 0}] == result_uint{140});
        CHECK(z[hmpc::index{0, 1, 1}] == result_uint{152});
        CHECK(z[hmpc::index{0, 1, 2}] == result_uint{164});
        CHECK(z[hmpc::index{0, 1, 3}] == result_uint{176});

        CHECK(z[hmpc::index{1, 0, 0}] == result_uint{23});
        CHECK(z[hmpc::index{1, 0, 1}] == result_uint{24});
        CHECK(z[hmpc::index{1, 0, 2}] == result_uint{25});
        CHECK(z[hmpc::index{1, 0, 3}] == result_uint{26});

        CHECK(z[hmpc::index{1, 1, 0}] == result_uint{27});
        CHECK(z[hmpc::index{1, 1, 1}] == result_uint{28});
        CHECK(z[hmpc::index{1, 1, 2}] == result_uint{29});
        CHECK(z[hmpc::index{1, 1, 3}] == result_uint{30});
    }
}
