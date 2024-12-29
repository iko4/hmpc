#include "catch_helpers.hpp"

#include <hmpc/core/uint.hpp>
#include <hmpc/ints/bigint.hpp>
#include <hmpc/ints/numeric.hpp>

TEST_CASE("Unsigned big integer operations", "[ints][bigint][uint]")
{
    constexpr hmpc::ints::ubigint<10> x = {42};
    REQUIRE(x.limb_size == 1);
    constexpr hmpc::ints::ubigint<3> y = {4};
    REQUIRE(y.limb_size == 1);

    auto add = x + y;
    REQUIRE(add.bit_size == 11);
    REQUIRE(add.limb_size == 1);
    REQUIRE(add.data[0].data == 46);

    auto subtract = x - y;
    REQUIRE(subtract.bit_size == 11);
    REQUIRE(subtract.limb_size == 1);
    REQUIRE(subtract.data[0].data == 38);

    auto multiply = x * y;
    REQUIRE(multiply.bit_size == 13);
    REQUIRE(multiply.limb_size == 1);
    REQUIRE(multiply.data[0].data == 168);

    auto bit_and = x & y;
    REQUIRE(bit_and.bit_size == 10);
    REQUIRE(bit_and.data[0].data == 0);

    auto bit_or = x | y;
    REQUIRE(bit_or.bit_size == 10);
    REQUIRE(bit_or.data[0].data == 46);

    auto bit_xor = x ^ y;
    REQUIRE(bit_xor.bit_size == 10);
    REQUIRE(bit_xor.data[0].data == 46);

    hmpc::iter::for_range<hmpc::size{10}>([&](auto i)
    {
        auto shifted_left = x << i;
        REQUIRE(shifted_left.bit_size == 10 + i);
        REQUIRE(shifted_left.data[0].data == 42 << i);
        auto shifted_right = x >> i;
        REQUIRE(shifted_right.bit_size == 10 - i);
        REQUIRE(shifted_right.data[0].data == 42 >> i);
    });

    auto divide = x / y;
    REQUIRE(divide.bit_size == 10);
    REQUIRE(divide.data[0].data == 10);

    auto mod = x % y;
    REQUIRE(mod.bit_size == 3);
    REQUIRE(mod.data[0].data == 2);
}

TEST_CASE("Unsigned big integer functions", "[ints][bigint]")
{
    SECTION("powers of two")
    {
        constexpr auto x = hmpc::ints::ubigint<10>{512};
        constexpr auto y = hmpc::ints::ubigint<10>{32};
        constexpr auto greatest_common_divisor_x_y = greatest_common_divisor(x, y);
        REQUIRE(greatest_common_divisor_x_y.data[0].data == 32);
    }

    SECTION("prime and power of two")
    {
        constexpr auto x = hmpc::ints::ubigint<64>{4'294'967'295, 536'870'911};                  // 2305843009213693951 = 2^61 - 1
        constexpr auto y = hmpc::ints::ubigint<64>{0, 0b10'0000'0000'0000'0000'0000'0000'0000u}; // 2^61 = 2305843009213693952
        constexpr auto greatest_common_divisor_x_y = greatest_common_divisor(x, y);
        REQUIRE(greatest_common_divisor_x_y == hmpc::ints::ubigint<1>{1});

        constexpr auto inverse = invert_modulo(x, y);
        REQUIRE(inverse == x);

        constexpr auto rinverse = invert_modulo(y, x);
        REQUIRE(rinverse == hmpc::ints::ubigint<1>{1});
    }
}

TEST_CASE("Big integer bit functions", "[ints][bigint]")
{
    SECTION("0")
    {
        hmpc::ints::ubigint<10> x = {};
        REQUIRE_FALSE(has_single_bit(x));
        REQUIRE(bit_width(x) == 0);

        hmpc::ints::sbigint<10> y = {};
        REQUIRE(bit_width(y) == 0);
    }

    SECTION("+-1")
    {
        hmpc::ints::ubigint<10> x = {1};
        REQUIRE(has_single_bit(x));
        REQUIRE(bit_width(x) == 1);

        hmpc::ints::sbigint<10> y = {1};
        REQUIRE(bit_width(y) == 1);

        hmpc::ints::sbigint<10> z = {-1};
        REQUIRE(bit_width(z) == 1);
    }

    SECTION("+-2")
    {
        hmpc::ints::ubigint<10> x = {2};
        REQUIRE(has_single_bit(x));
        REQUIRE(bit_width(x) == 2);

        hmpc::ints::sbigint<10> y = {2};
        REQUIRE(bit_width(y) == 2);

        hmpc::ints::sbigint<10> z = {-2};
        REQUIRE(bit_width(z) == 2);
    }

    SECTION("+-7")
    {
        hmpc::ints::ubigint<10> x = {7};
        REQUIRE_FALSE(has_single_bit(x));
        REQUIRE(bit_width(x) == 3);

        hmpc::ints::sbigint<10> y = {7};
        REQUIRE(bit_width(y) == 3);

        hmpc::ints::sbigint<10> z = {-7};
        REQUIRE(bit_width(z) == 4);
    }

    SECTION("42")
    {
        hmpc::ints::ubigint<10> x = {42};
        REQUIRE_FALSE(has_single_bit(x));
        REQUIRE(bit_width(x) == 6);

        hmpc::ints::ubigint<100> y = {42, 0, 0};
        REQUIRE_FALSE(has_single_bit(y));
        REQUIRE(bit_width(y) == 6);
    }

    SECTION("32")
    {
        hmpc::ints::ubigint<10> x = {32};
        REQUIRE(has_single_bit(x));
        REQUIRE(bit_width(x) == 6);

        hmpc::ints::ubigint<100> y = {32};
        REQUIRE(has_single_bit(y));
        REQUIRE(bit_width(y) == 6);
    }

    SECTION("8 0 8")
    {
        hmpc::ints::ubigint<100> x = {8, 0, 8};
        REQUIRE_FALSE(has_single_bit(x));
        REQUIRE(bit_width(x) == 68);
    }

    SECTION("-12")
    {
        hmpc::ints::sbigint<16> x = {-12};
        REQUIRE(bit_width(x) == 5);
    }
}

TEST_CASE("Signed big integer operations", "[ints][bigint][uint]")
{
    constexpr auto x = hmpc::ints::sbigint<7>{0xffff'ffd6}; // -42
    constexpr auto y = hmpc::ints::sbigint<3>{0xffff'fffc}; // -4

    REQUIRE(x < hmpc::ints::zero<>);
    REQUIRE(x < hmpc::ints::one<>);

    REQUIRE(-x > hmpc::ints::zero<>);
    REQUIRE(-x > hmpc::ints::one<>);

    REQUIRE(x < y);
    REQUIRE(x <= y);
    REQUIRE(-x > -y);
}
