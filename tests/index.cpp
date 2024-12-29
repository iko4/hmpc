#include "catch_helpers.hpp"

#include <hmpc/index.hpp>
#include <hmpc/constants.hpp>

TEST_CASE("Index", "[shape][index]")
{
    constexpr hmpc::size N = 10;
    constexpr hmpc::size M = 11;

    SECTION("1D")
    {
        auto shape = hmpc::shape{N};
        for (hmpc::size n = 0; n < N; ++n)
        {
            auto i = n;
            auto j = hmpc::from_linear_index(i, shape);
            auto k = hmpc::to_linear_index(j, shape);
            REQUIRE(i == k);

            auto [x] = j;
            REQUIRE(x == n);
        }
    }

    SECTION("2D")
    {
        auto shape = hmpc::shape{N, M};
        for (hmpc::size n = 0; n < N; ++n)
        {
            for (hmpc::size m = 0; m < M; ++m)
            {
                auto i = m + M * n;
                auto j = hmpc::from_linear_index(i, shape);
                auto k = hmpc::to_linear_index(j, shape);
                REQUIRE(i == k);

                j = hmpc::index{n, m};
                k = hmpc::to_linear_index(j, shape);
                REQUIRE(i == k);

                auto [x, y] = j;
                REQUIRE(x == n);
                REQUIRE(y == m);
            }
        }
    }

    SECTION("2D with placeholder")
    {
        auto shape = hmpc::shape{N, hmpc::constants::placeholder};
        auto full_shape = hmpc::shape{N, M};
        for (hmpc::size n = 0; n < N; ++n)
        {
            for (hmpc::size m = 0; m < M; ++m)
            {
                auto i = m + M * n;
                auto j = hmpc::from_linear_index(i, full_shape);
                auto k = hmpc::to_linear_index(hmpc::index{j}, shape);
                REQUIRE(k == n);
            }
        }
    }

    SECTION("2D with placeholder")
    {
        auto shape = hmpc::shape{hmpc::constants::placeholder, M};
        auto full_shape = hmpc::shape{N, M};
        for (hmpc::size n = 0; n < N; ++n)
        {
            for (hmpc::size m = 0; m < M; ++m)
            {
                auto i = m + M * n;
                auto j = hmpc::from_linear_index(i, full_shape);
                auto k = hmpc::to_linear_index(hmpc::index{j}, shape);
                REQUIRE(k == m);
            }
        }
    }
}
