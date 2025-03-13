#include "catch_helpers.hpp"

#include <hmpc/constants.hpp>
#include <hmpc/range.hpp>

TEST_CASE("Range", "[range]")
{
    constexpr auto x = hmpc::size_constant_of<42>;
    constexpr auto y = hmpc::size_constant_of<2>;

    constexpr auto r = hmpc::range(x);

    REQUIRE(r.start == 0);
    REQUIRE(r.end == 42);
    REQUIRE(r.step == 1);
    CHECK(r.contains(hmpc::constants::zero));
    CHECK_FALSE(r.contains(x));
    CHECK(r.contains(y));

    constexpr auto s = hmpc::range(y, x);

    REQUIRE(s.start == 4);
    REQUIRE(s.end == 42);
    REQUIRE(r.step == 1);
    CHECK_FALSE(r.contains(hmpc::constants::zero));
    CHECK_FALSE(r.contains(x));
    CHECK(r.contains(y));
}
