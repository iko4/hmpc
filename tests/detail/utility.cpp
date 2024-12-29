#include "catch_helpers.hpp"

#include <hmpc/detail/utility.hpp>

TEST_CASE("Implication", "[utility]")
{
    REQUIRE(hmpc::detail::implies(false, false) == true);
    REQUIRE(hmpc::detail::implies(false, true) == true);
    REQUIRE(hmpc::detail::implies(true, false) == false);
    REQUIRE(hmpc::detail::implies(true, true) == true);
}
