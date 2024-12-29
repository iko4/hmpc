#include "catch_helpers.hpp"

#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/queue.hpp>
#include <hmpc/expr/bit_monomial.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/expr/matrix_vector_product.hpp>
#include <hmpc/ints/bigint.hpp>

TEST_CASE("Matrix-vector multiplication of polynomials", "[expr][matmul][poly]")
{
    constexpr hmpc::size N = 1 << 3;
    constexpr auto U = hmpc::constants::four;
    constexpr hmpc::size V = 2;
    constexpr auto x_shape = hmpc::shape{2, U};
    constexpr auto index_shape = hmpc::shape{2, V, U};

    using uint = hmpc::ints::ubigint<7>;
    using R = hmpc::ints::poly<uint, N>;

    auto index_tensor = hmpc::comp::make_tensor<std::optional<hmpc::size>>(index_shape);
    {
        hmpc::comp::host_accessor indices(index_tensor, hmpc::access::discard_write);

        indices[hmpc::index{0, 0, 0}] = 1;
        indices[hmpc::index{0, 0, 1}] = std::nullopt;
        indices[hmpc::index{0, 0, 2}] = 14;
        indices[hmpc::index{0, 0, 3}] = 9;
        indices[hmpc::index{0, 1, 0}] = 6;
        indices[hmpc::index{0, 1, 1}] = 5;
        indices[hmpc::index{0, 1, 2}] = 15;
        indices[hmpc::index{0, 1, 3}] = 8;

        indices[hmpc::index{1, 0, 0}] = 3;
        indices[hmpc::index{1, 0, 1}] = 10;
        indices[hmpc::index{1, 0, 2}] = std::nullopt;
        indices[hmpc::index{1, 0, 3}] = 12;
        indices[hmpc::index{1, 1, 0}] = 0;
        indices[hmpc::index{1, 1, 1}] = 4;
        indices[hmpc::index{1, 1, 2}] = 7;
        indices[hmpc::index{1, 1, 3}] = 2;
    }
    auto x_tensor = hmpc::comp::make_tensor<R>(x_shape);
    {
        hmpc::comp::host_accessor x(x_tensor, hmpc::access::discard_write);

        x[hmpc::index{0, 0, 0}] = uint{1};
        x[hmpc::index{0, 0, 1}] = uint{2};
        x[hmpc::index{0, 0, 2}] = uint{3};
        x[hmpc::index{0, 0, 3}] = uint{4};
        x[hmpc::index{0, 0, 4}] = uint{5};
        x[hmpc::index{0, 0, 5}] = uint{6};
        x[hmpc::index{0, 0, 6}] = uint{7};
        x[hmpc::index{0, 0, 7}] = uint{8};
        x[hmpc::index{0, 1, 0}] = uint{9};
        x[hmpc::index{0, 1, 1}] = uint{10};
        x[hmpc::index{0, 1, 2}] = uint{11};
        x[hmpc::index{0, 1, 3}] = uint{12};
        x[hmpc::index{0, 1, 4}] = uint{13};
        x[hmpc::index{0, 1, 5}] = uint{14};
        x[hmpc::index{0, 1, 6}] = uint{15};
        x[hmpc::index{0, 1, 7}] = uint{16};
        x[hmpc::index{0, 2, 0}] = uint{17};
        x[hmpc::index{0, 2, 1}] = uint{18};
        x[hmpc::index{0, 2, 2}] = uint{19};
        x[hmpc::index{0, 2, 3}] = uint{20};
        x[hmpc::index{0, 2, 4}] = uint{21};
        x[hmpc::index{0, 2, 5}] = uint{22};
        x[hmpc::index{0, 2, 6}] = uint{23};
        x[hmpc::index{0, 2, 7}] = uint{24};
        x[hmpc::index{0, 3, 0}] = uint{25};
        x[hmpc::index{0, 3, 1}] = uint{26};
        x[hmpc::index{0, 3, 2}] = uint{27};
        x[hmpc::index{0, 3, 3}] = uint{28};
        x[hmpc::index{0, 3, 4}] = uint{29};
        x[hmpc::index{0, 3, 5}] = uint{30};
        x[hmpc::index{0, 3, 6}] = uint{31};
        x[hmpc::index{0, 3, 7}] = uint{32};

        x[hmpc::index{1, 0, 0}] = uint{33};
        x[hmpc::index{1, 0, 1}] = uint{34};
        x[hmpc::index{1, 0, 2}] = uint{35};
        x[hmpc::index{1, 0, 3}] = uint{36};
        x[hmpc::index{1, 0, 4}] = uint{37};
        x[hmpc::index{1, 0, 5}] = uint{38};
        x[hmpc::index{1, 0, 6}] = uint{39};
        x[hmpc::index{1, 0, 7}] = uint{40};
        x[hmpc::index{1, 1, 0}] = uint{41};
        x[hmpc::index{1, 1, 1}] = uint{42};
        x[hmpc::index{1, 1, 2}] = uint{43};
        x[hmpc::index{1, 1, 3}] = uint{44};
        x[hmpc::index{1, 1, 4}] = uint{45};
        x[hmpc::index{1, 1, 5}] = uint{46};
        x[hmpc::index{1, 1, 6}] = uint{47};
        x[hmpc::index{1, 1, 7}] = uint{48};
        x[hmpc::index{1, 2, 0}] = uint{49};
        x[hmpc::index{1, 2, 1}] = uint{50};
        x[hmpc::index{1, 2, 2}] = uint{51};
        x[hmpc::index{1, 2, 3}] = uint{52};
        x[hmpc::index{1, 2, 4}] = uint{53};
        x[hmpc::index{1, 2, 5}] = uint{54};
        x[hmpc::index{1, 2, 6}] = uint{55};
        x[hmpc::index{1, 2, 7}] = uint{56};
        x[hmpc::index{1, 3, 0}] = uint{57};
        x[hmpc::index{1, 3, 1}] = uint{58};
        x[hmpc::index{1, 3, 2}] = uint{59};
        x[hmpc::index{1, 3, 3}] = uint{60};
        x[hmpc::index{1, 3, 4}] = uint{61};
        x[hmpc::index{1, 3, 5}] = uint{62};
        x[hmpc::index{1, 3, 6}] = uint{63};
        x[hmpc::index{1, 3, 7}] = uint{64};
    }

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    auto index = hmpc::expr::tensor(index_tensor);
    auto monomial = hmpc::expr::bit_monomial<N>(index);
    auto x = hmpc::expr::tensor(x_tensor);

    using integer = hmpc::ints::sbigint<9>;

    auto z_tensor = queue(hmpc::expr::matrix_vector_product(monomial, x));
    auto z_shape = z_tensor.shape();
    REQUIRE(z_shape.rank == 2);
    REQUIRE(z_shape.get(hmpc::constants::zero) == 2);
    REQUIRE(z_shape.get(hmpc::constants::one) == V);
    {
        hmpc::comp::host_accessor z(z_tensor, hmpc::access::read);
        CHECK(z[hmpc::index{0, 0, 0}] == integer{43});
        CHECK(z[hmpc::index{0, 0, 1}] == -integer{4});
        CHECK(z[hmpc::index{0, 0, 2}] == -integer{3});
        CHECK(z[hmpc::index{0, 0, 3}] == -integer{2});
        CHECK(z[hmpc::index{0, 0, 4}] == -integer{1});
        CHECK(z[hmpc::index{0, 0, 5}] == integer{0});
        CHECK(z[hmpc::index{0, 0, 6}] == -integer{41});
        CHECK(z[hmpc::index{0, 0, 7}] == -integer{42});

        CHECK(z[hmpc::index{0, 1, 0}] == -integer{22});
        CHECK(z[hmpc::index{0, 1, 1}] == -integer{24});
        CHECK(z[hmpc::index{0, 1, 2}] == -integer{26});
        CHECK(z[hmpc::index{0, 1, 3}] == -integer{28});
        CHECK(z[hmpc::index{0, 1, 4}] == -integer{30});
        CHECK(z[hmpc::index{0, 1, 5}] == -integer{6});
        CHECK(z[hmpc::index{0, 1, 6}] == integer{4});
        CHECK(z[hmpc::index{0, 1, 7}] == -integer{36});

        CHECK(z[hmpc::index{1, 0, 0}] == integer{70});
        CHECK(z[hmpc::index{1, 0, 1}] == integer{71});
        CHECK(z[hmpc::index{1, 0, 2}] == -integer{18});
        CHECK(z[hmpc::index{1, 0, 3}] == integer{55});
        CHECK(z[hmpc::index{1, 0, 4}] == -integer{66});
        CHECK(z[hmpc::index{1, 0, 5}] == -integer{67});
        CHECK(z[hmpc::index{1, 0, 6}] == -integer{68});
        CHECK(z[hmpc::index{1, 0, 7}] == -integer{69});

        CHECK(z[hmpc::index{1, 1, 0}] == -integer{125});
        CHECK(z[hmpc::index{1, 1, 1}] == -integer{127});
        CHECK(z[hmpc::index{1, 1, 2}] == -integer{7});
        CHECK(z[hmpc::index{1, 1, 3}] == -integer{7});
        CHECK(z[hmpc::index{1, 1, 4}] == integer{83});
        CHECK(z[hmpc::index{1, 1, 5}] == integer{85});
        CHECK(z[hmpc::index{1, 1, 6}] == integer{87});
        CHECK(z[hmpc::index{1, 1, 7}] == integer{195});
    }
}
