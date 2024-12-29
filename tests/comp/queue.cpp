#include "catch_helpers.hpp"

#include <hmpc/comp/queue.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/expr/cache.hpp>
#include <hmpc/ints/uint.hpp>

#include <sycl/sycl.hpp>

TEST_CASE("Queue", "[comp][expr]")
{
    using uint = hmpc::ints::uint<64>;
    using limb = uint::limb_type;

    constexpr hmpc::size N = 10;

    auto x = hmpc::comp::make_tensor<uint>(hmpc::shape{N, hmpc::constants::placeholder});
    auto y = hmpc::comp::make_tensor<uint>(hmpc::shape{hmpc::constants::placeholder, N});

    REQUIRE(x.shape().size() == N);
    REQUIRE(y.shape().size() == N);

    using namespace hmpc::expr::operators;

    auto expr = hmpc::expr::tensor(x) + hmpc::expr::tensor(y);

    REQUIRE_FALSE(hmpc::expr::same_element_shape<decltype(expr)>);
    REQUIRE_FALSE(hmpc::expr::same_element_shape<decltype(hmpc::expr::cache(expr))>);

    {
        hmpc::comp::host_accessor access_x(x, hmpc::access::discard_write);
        hmpc::comp::host_accessor access_y(y, hmpc::access::discard_write);

        for (hmpc::size i = 0; i < N; ++i)
        {
            access_x[i] = uint{static_cast<limb>(3 * i + 1)};
            access_y[i] = uint{static_cast<limb>(i + 1)};
        }
    }

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    auto z = queue(expr);
    REQUIRE(z.shape().rank == 2);
    REQUIRE(z.shape().size() == N * N);

    hmpc::comp::host_accessor access_z(z, hmpc::access::read);
    for (hmpc::size i = 0; i < N; ++i)
    {
        for (hmpc::size j = 0; j < N; ++j)
        {
            auto index = hmpc::index{i, j};
            auto expected = uint{static_cast<limb>(3 * i + j + 2)};
            auto observed = access_z[index];
            CHECK(expected == observed);
        }
    }

    // some requirements on device information are given here: https://registry.khronos.org/SYCL/specs/sycl-2020/html/sycl-2020.html#_device_information_descriptors
    auto info = queue.info();
    CHECK(info.type == hmpc::comp::device_type::cpu);
    CHECK(info.available);
    CHECK((info.address_bits == 32) bitor (info.address_bits == 64));
    CHECK(info.limits.compute_units >= 1);
    CHECK(info.limits.sub_devices <= info.limits.compute_units);
    CHECK(info.limits.work_item_dimensions >= 3); // if not custom device
    CHECK(info.limits.work_item_size_1d >= 1); // if not custom device
    CHECK(info.limits.work_item_size_2d >= std::array<std::size_t, 2>{1, 1}); // if not custom device
    CHECK(info.limits.work_item_size_3d >= std::array<std::size_t, 3>{1, 1, 1}); // if not custom device
    CHECK(info.limits.work_group_size >= 1);
    CHECK(info.limits.sub_groups >= 1);
    CHECK(info.limits.parameter_size >= 1024); // if not custom device
    CHECK(info.limits.local_memory_size >= 32 * 1000); // if not custom device
}
