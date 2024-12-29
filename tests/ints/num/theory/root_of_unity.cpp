#include "catch_helpers.hpp"

#include <hmpc/ints/literals.hpp>
#include <hmpc/ints/num/theory/root_of_unity.hpp>

TEST_CASE("Root of unity", "[number_theory]")
{
    using namespace hmpc::ints::literals;
    constexpr auto p = 0b110'0100'0001_int; // 1601
    constexpr hmpc::size n = 1 << 6;

    auto root = hmpc::ints::num::theory::root_of_unity(hmpc::constant_of<p>, hmpc::constant_of<n>);
    REQUIRE(hmpc::ints::num::theory::is_root_of_unity(root, hmpc::constant_of<n>));

    using mod = decltype(root);
    using limb = mod::limb_type;

    constexpr auto one = mod{hmpc::ints::one<limb>};

    hmpc::iter::for_range<hmpc::size{1}, n - 1>([&](auto i)
    {
        REQUIRE(pow(root, i) != one);
    });

    auto root_pow_0 = pow(root, hmpc::constants::zero);
    REQUIRE(root_pow_0 == one);
    auto root_pow_n = pow(root, hmpc::constant_of<n>);
    REQUIRE(root_pow_n == one);
}
