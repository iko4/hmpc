#include "catch_helpers.hpp"

#include <hmpc/constants.hpp>
#include <hmpc/core/mdsize.hpp>

TEST_CASE("Multidimensional size", "[core][mdsize]")
{
    SECTION("0D")
    {
        auto size = hmpc::core::mdsize{};
        REQUIRE(size.rank == 0);
        REQUIRE(size.static_rank == 0);
        REQUIRE(size.dynamic_rank == 0);
    }

    SECTION("1D dynamic")
    {
        auto size = hmpc::core::mdsize{42u};
        auto dim0 = size.get(hmpc::constants::zero);
        REQUIRE(size.rank == 1);
        REQUIRE(size.static_rank == 0);
        REQUIRE(size.dynamic_rank == 1);
        REQUIRE(dim0 == 42);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(size.extent(hmpc::constants::zero) == hmpc::dynamic_extent);
    }

    SECTION("1D fixed")
    {
        auto size = hmpc::core::mdsize<42>{};
        auto dim0 = size.get(hmpc::constants::zero);
        REQUIRE(size.rank == 1);
        REQUIRE(size.static_rank == 1);
        REQUIRE(size.dynamic_rank == 0);
        REQUIRE(dim0 == 42);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(size.extent(hmpc::constants::zero) == 42);
    }

    SECTION("1D fixed")
    {
        auto size = hmpc::core::mdsize{hmpc::size_constant_of<42>};
        auto dim0 = size.get(hmpc::constants::zero);
        REQUIRE(size.rank == 1);
        REQUIRE(size.static_rank == 1);
        REQUIRE(size.dynamic_rank == 0);
        REQUIRE(dim0 == 42);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(size.extent(hmpc::constants::zero) == 42);
    }
}
