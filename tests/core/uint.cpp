#include "catch_helpers.hpp"

#include <hmpc/core/uint.hpp>

#include <array>

TEST_CASE("Core integer types are present", "[core][uint]")
{
    REQUIRE((std::same_as<hmpc::core::uint1::underlying_type, bool>) == true);
    REQUIRE((std::same_as<hmpc::core::uint8::underlying_type, std::uint8_t>) == true);
    REQUIRE((std::same_as<hmpc::core::uint16::underlying_type, std::uint16_t>) == true);
    REQUIRE((std::same_as<hmpc::core::uint32::underlying_type, std::uint32_t>) == true);
    REQUIRE((std::same_as<hmpc::core::uint64::underlying_type, std::uint64_t>) == true);
}

TEST_CASE("Core integer construction", "[core][uint]")
{
    hmpc::core::uint1 u1{false};
    hmpc::core::uint8 u8{42};
    hmpc::core::uint16 u16{42};
    hmpc::core::uint32 u32{42};
    hmpc::core::uint64 u64{42};

    REQUIRE(u1.data == false);
    REQUIRE(u8.data == 42);
    REQUIRE(u16.data == 42);
    REQUIRE(u32.data == 42);
    REQUIRE(u64.data == 42);

    SECTION("Conversion to u1")
    {
        REQUIRE(hmpc::core::uint1{u8} == u1);
        REQUIRE(hmpc::core::uint1{u16} == u1);
        REQUIRE(hmpc::core::uint1{u32} == u1);
        REQUIRE(hmpc::core::uint1{u64} == u1);
    }

    SECTION("Conversion to u8")
    {
        REQUIRE(hmpc::core::uint8{u1} == hmpc::core::uint8{});
        REQUIRE(hmpc::core::uint8{u16} == u8);
        REQUIRE(hmpc::core::uint8{u32} == u8);
        REQUIRE(hmpc::core::uint8{u64} == u8);
    }

    SECTION("Conversion to u16")
    {
        REQUIRE(hmpc::core::uint16{u1} == hmpc::core::uint16{});
        REQUIRE(hmpc::core::uint16{u8} == u16);
        REQUIRE(hmpc::core::uint16{u32} == u16);
        REQUIRE(hmpc::core::uint16{u64} == u16);
    }

    SECTION("Conversion to u32")
    {
        REQUIRE(hmpc::core::uint32{u1} == hmpc::core::uint32{});
        REQUIRE(hmpc::core::uint32{u8} == u32);
        REQUIRE(hmpc::core::uint32{u16} == u32);
        REQUIRE(hmpc::core::uint32{u64} == u32);
    }

    SECTION("Conversion to u64")
    {
        REQUIRE(hmpc::core::uint64{u1} == hmpc::core::uint64{});
        REQUIRE(hmpc::core::uint64{u8} == u64);
        REQUIRE(hmpc::core::uint64{u16} == u64);
        REQUIRE(hmpc::core::uint64{u32} == u64);
    }
}

TEST_CASE("Core integer operations", "[core][uint]")
{
    auto zero = hmpc::core::uint1{};
    auto one = hmpc::core::uint1{true};

    SECTION("u1 nullary operations")
    {
        REQUIRE(zero.data == false);
        REQUIRE(one.data == true);

        REQUIRE_FALSE(zero);
        REQUIRE(one);
    }

    SECTION("u1 unary operations")
    {
        REQUIRE((~zero).data == true);
        REQUIRE((~one).data == false);
        REQUIRE((-zero).data == false);
        REQUIRE((-one).data == true);
        REQUIRE((!zero).data == true);
        REQUIRE((!one).data == false);

        REQUIRE(static_cast<bool>(zero) == false);
        REQUIRE(static_cast<bool>(one) == true);

        REQUIRE(static_cast<hmpc::size>(zero) == 0);
        REQUIRE(static_cast<hmpc::size>(one) == 1);
    }

    SECTION("u1 binary operations")
    {
        std::array<std::array<bool, 2 + 12>, 4> ground_truth =
        {{
            // left right == != < <= > >= + - * & | ^
            {0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0},
            {0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1},
            {1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1},
            {1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0},
        }};

        for (auto const& row : ground_truth)
        {
            auto left = hmpc::core::uint1{row[0]};
            auto right = hmpc::core::uint1{row[1]};
            REQUIRE((left == right).data == row[2]);
            REQUIRE((left != right).data == row[3]);
            REQUIRE((left < right).data == row[4]);
            REQUIRE((left <= right).data == row[5]);
            REQUIRE((left > right).data == row[6]);
            REQUIRE((left >= right).data == row[7]);
            REQUIRE((left + right).data == row[8]);
            REQUIRE((left - right).data == row[9]);
            REQUIRE((left * right).data == row[10]);
            REQUIRE((left & right).data == row[11]);
            REQUIRE((left | right).data == row[12]);
            REQUIRE((left ^ right).data == row[13]);
        }
    }

    SECTION("u8 unary operations")
    {
        for (std::uint32_t x = 0; x <= 1024; ++x)
        {
            auto value = hmpc::core::uint8(hmpc::core::uint32(x));

            REQUIRE((~value).data == ((~x) & 0xFFU));
            REQUIRE((-value).data == ((-x) & 0xFFU));
        }
    }

    SECTION("u8 binary operations")
    {
        for (std::uint32_t x = 0; x <= 255; ++x)
        {
            auto left = hmpc::core::uint8(hmpc::core::uint32(x));
            REQUIRE((-left).data == ((-x) & 0xFFU));
            REQUIRE((~left).data == ((~x) & 0xFFU));
            for (std::uint32_t y = 0; y <= 255; ++y)
            {
                auto right = hmpc::core::uint8(hmpc::core::uint32(y));
                REQUIRE((left == right).data == (x == y));
                REQUIRE((left != right).data == (x != y));
                REQUIRE((left < right).data == (x < y));
                REQUIRE((left <= right).data == (x <= y));
                REQUIRE((left > right).data == (x > y));
                REQUIRE((left >= right).data == (x >= y));
                REQUIRE((left + right).data == ((x + y) & 0xFFU));
                REQUIRE((left - right).data == ((x - y) & 0xFFU));
                REQUIRE((left * right).data == ((x * y) & 0xFFU));
                REQUIRE((left & right).data == ((x & y) & 0xFFU));
                REQUIRE((left | right).data == ((x | y) & 0xFFU));
                REQUIRE((left ^ right).data == ((x ^ y) & 0xFFU));
            }
        }
    }
}
