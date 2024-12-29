#include "catch_helpers.hpp"

#include <hmpc/ints/literals.hpp>
#include <hmpc/ints/numeric.hpp>
#include <hmpc/random/number_generator.hpp>
#include <hmpc/random/uniform.hpp>

TEST_CASE("Uniform", "[random]")
{
    auto rng = hmpc::random::compiletime_number_generator();
    using namespace hmpc::ints::literals;

    auto bits = hmpc::size_constant_of<42>;
    auto bound = hmpc::constant_of<42_int>;

    SECTION("Uniform unsigned integer")
    {
        auto x = hmpc::random::unsigned_uniform(rng, bits);
        REQUIRE_FALSE(hmpc::is_signed(x.signedness));
        REQUIRE(x.bit_size == bits);
    }

    SECTION("Uniform signed integer")
    {
        auto x = hmpc::random::signed_uniform(rng, bits);
        REQUIRE(hmpc::is_signed(x.signedness));
        REQUIRE(x.bit_size == bits);
    }

    SECTION("Uniform drowned unsigned integer")
    {
        auto x = hmpc::random::drown_unsigned_uniform(rng, bound);
        REQUIRE_FALSE(hmpc::is_signed(x.signedness));
        REQUIRE(x.bit_size == 6 + static_cast<hmpc::size>(hmpc::default_statistical_security));
    }

    SECTION("Uniform drowned signed integer")
    {
        auto x = hmpc::random::drown_signed_uniform(rng, bound);
        REQUIRE(hmpc::is_signed(x.signedness));
        REQUIRE(x.bit_size == 6 + static_cast<hmpc::size>(hmpc::default_statistical_security) + 1);
    }

    SECTION("Uniform integer modulo")
    {
        constexpr auto p = 0x2faeadbe7a0195c011ac195ad10269830e8001_int; // 1'063'351'684'119'354'455'646'439'919'975'158'459'206'107'137

        using mod_p = hmpc::ints::mod<p>;

        auto x = hmpc::random::uniform<mod_p>(rng);
        REQUIRE(x.modulus == p);
    }
}
