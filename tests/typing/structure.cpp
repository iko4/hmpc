#include "catch_helpers.hpp"

#include <hmpc/comp/crypto/lhe/ciphertext.hpp>
#include <hmpc/ints/literals.hpp>
#include <hmpc/ints/poly_mod.hpp>
#include <hmpc/iter/at_structure.hpp>
#include <hmpc/iter/collect_structure.hpp>
#include <hmpc/iter/enumerate_structure.hpp>
#include <hmpc/typing/structure.hpp>

template<typename Left, typename Right>
struct structured
{
    Left left;
    Right right;

    using is_structure = void;
    static constexpr hmpc::size size = 2;

    constexpr auto& get(hmpc::size_constant<0>) const
    {
        return left;
    }

    constexpr auto& get(hmpc::size_constant<1>) const
    {
        return right;
    }
};

struct visitor
{
    constexpr std::string_view operator()(int) const
    {
        return "int";
    }

    constexpr std::string_view operator()(float) const
    {
        return "float";
    }

    std::string_view operator()(std::string) const
    {
        return "string";
    }

    constexpr std::string_view operator()(auto) const
    {
        return "other";
    }
};

TEST_CASE("Structure", "[typing][structure]")
{
    using namespace hmpc::ints::literals;
    using Rq = hmpc::ints::poly_mod<11_int, 3, hmpc::ints::coefficient_representation>;
    using C = hmpc::comp::crypto::lhe::ciphertext<Rq, 100>;

    using recursive = structured<structured<int, float>, std::string>;

    REQUIRE(hmpc::structure<C>);
    REQUIRE(hmpc::structure<recursive>);
    REQUIRE_FALSE(hmpc::structure<int>);

    REQUIRE(hmpc::typing::traits::structure_fields_v<Rq> == 1);
    REQUIRE(hmpc::typing::traits::structure_fields_v<C> == 2);
    REQUIRE(hmpc::typing::traits::structure_fields_v<recursive> == 3);

    recursive r = {};
    visitor v = {};
    hmpc::size counter = 0;

    std::array<std::string_view, 3> strings = {};
    hmpc::iter::enumerate(r, [&](auto i, auto value)
    {
        REQUIRE(i >= 0);
        REQUIRE(i < 3);
        REQUIRE(counter == i);
        strings[i] = v(value);
        ++counter;
    });
    REQUIRE(counter == 3);

    REQUIRE(strings[0] == "int");
    REQUIRE(strings[1] == "float");
    REQUIRE(strings[2] == "string");

    auto tuple = hmpc::iter::collect(r, [](auto value){ return value; });
    REQUIRE(std::same_as<decltype(tuple), std::tuple<int, float, std::string>>);

    auto array = hmpc::iter::collect(r, [&](auto value){ return v(value); });
    REQUIRE(std::same_as<decltype(array), std::array<std::string_view, 3>>);
    REQUIRE(array == strings);

    REQUIRE(v(hmpc::iter::at(r, hmpc::constants::two)) == "string");
    REQUIRE(v(hmpc::iter::at(r, hmpc::constants::zero)) == "int");
    REQUIRE(v(hmpc::iter::at(r, hmpc::constants::one)) == "float");
}
