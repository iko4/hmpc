#include "catch_helpers.hpp"

#include <hmpc/ints/literals.hpp>

TEST_CASE("Unsigned big integer literals", "[bigint][literals]")
{
    using namespace hmpc::ints::literals;

    auto x_binary = 0b0000'1011_int;
    REQUIRE(x_binary.bit_size == 4);
    REQUIRE(x_binary.data[0].data == 11);

    auto x_hex = 0x00042_int;
    REQUIRE(x_hex.bit_size == 7);
    REQUIRE(x_hex.data[0].data == 66);

    auto x_decimal = 1234_int;
    REQUIRE(x_decimal.bit_size == 11);
    REQUIRE(x_decimal.data[0].data == 1234);
}
