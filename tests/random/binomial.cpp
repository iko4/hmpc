#include "catch_helpers.hpp"

#include <hmpc/ints/numeric.hpp>
#include <hmpc/random/binomial.hpp>
#include <hmpc/random/number_generator.hpp>

TEST_CASE("Binomial", "[random]")
{
    auto rng = hmpc::random::compiletime_number_generator();

    REQUIRE(hmpc::random::binomial_limits<1, decltype(rng)>::bit_count == 1);
    REQUIRE(hmpc::random::binomial_limits<1, decltype(rng)>::bit_size == 1);
    REQUIRE(hmpc::random::binomial_limits<1, decltype(rng)>::min == hmpc::ints::zero<>);
    REQUIRE(hmpc::random::binomial_limits<1, decltype(rng)>::max == hmpc::ints::one<>);

    REQUIRE(hmpc::random::binomial_limits<10, decltype(rng)>::bit_count == 10);
    REQUIRE(hmpc::random::binomial_limits<10, decltype(rng)>::bit_size == 4);
    REQUIRE(hmpc::random::binomial_limits<10, decltype(rng)>::min == hmpc::ints::zero<>);
    REQUIRE(hmpc::random::binomial_limits<10, decltype(rng)>::max == hmpc::ints::ubigint<4>{10});

    REQUIRE(hmpc::random::centered_binomial_limits<10, decltype(rng)>::bit_count == 40);
    REQUIRE(hmpc::random::centered_binomial_limits<10, decltype(rng)>::bit_size == 6);
    REQUIRE(hmpc::random::centered_binomial_limits<10, decltype(rng)>::min == -hmpc::ints::ubigint<5>{20});
    REQUIRE(hmpc::random::centered_binomial_limits<10, decltype(rng)>::max == hmpc::ints::ubigint<5>{20});

    REQUIRE(hmpc::random::centered_binomial_limits<hmpc::rational_size{1, 2}, decltype(rng)>::bit_count == 2);
    REQUIRE(hmpc::random::centered_binomial_limits<hmpc::rational_size{1, 2}, decltype(rng)>::bit_size == 2);
    REQUIRE(hmpc::random::centered_binomial_limits<hmpc::rational_size{1, 2}, decltype(rng)>::min == -hmpc::ints::one<>);
    REQUIRE(hmpc::random::centered_binomial_limits<hmpc::rational_size{1, 2}, decltype(rng)>::max == hmpc::ints::one<>);

    for (int i = 0; i < 100'000; ++i)
    {
        auto b = hmpc::random::binomial<1>(rng);
        REQUIRE_FALSE(hmpc::is_signed(b.signedness));
        REQUIRE(b.bit_size == 1);
        CHECK(b <= hmpc::ints::one<>);

        auto c = hmpc::random::binomial<10>(rng);
        REQUIRE_FALSE(hmpc::is_signed(c.signedness));
        REQUIRE(c.bit_size == 4);
        CHECK(c <= hmpc::ints::ubigint<4>{10});

        auto d = hmpc::random::centered_binomial(rng, hmpc::constants::ten);
        REQUIRE(hmpc::is_signed(d.signedness));
        REQUIRE(d.bit_size == 6);
        CHECK(hmpc::ints::sbigint<6>{-20} <= d);
        CHECK(d <= hmpc::ints::ubigint<5>{20});
        CHECK(abs(d) <= hmpc::ints::ubigint<5>{20});

        auto e = hmpc::random::centered_binomial(rng, hmpc::constants::half);
        REQUIRE(hmpc::is_signed(e.signedness));
        REQUIRE(e.bit_size == 2);
        CHECK(hmpc::ints::sbigint<1>{-1} <= e);
        CHECK(e <= hmpc::ints::ubigint<1>{1});
        CHECK(abs(e) <= hmpc::ints::ubigint<1>{1});
    }
}
