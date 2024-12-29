#include "catch_helpers.hpp"

#include <hmpc/constants.hpp>
#include <hmpc/detail/type_set.hpp>

struct struct_with_reference
{
    int value;
    int& reference;
};

TEST_CASE("Type set", "[type_set]")
{
    SECTION("Empty set")
    {
        auto set = hmpc::detail::type_set{};
        REQUIRE(set.size == 0);
        REQUIRE_FALSE(set.contains<int>());
        REQUIRE_FALSE(set.contains<double>());
    }

    SECTION("One element")
    {
        auto set = hmpc::detail::type_set<int>{42};
        REQUIRE(set.size == 1);
        REQUIRE(set.contains<int>());
        REQUIRE_FALSE(set.contains<double>());
        REQUIRE(set.get(hmpc::constants::zero) == 42);
        REQUIRE(set.get(hmpc::detail::tag_of<int>) == 42);
    }

    SECTION("Two elements")
    {
        auto set = hmpc::detail::type_set<int, double>{42};
        REQUIRE(set.size == 2);
        REQUIRE(set.contains<int>());
        REQUIRE(set.contains<double>());
        REQUIRE(set.get(hmpc::constants::zero) == 42);
        REQUIRE(set.get(hmpc::detail::tag_of<int>) == 42);
        REQUIRE(set.get(hmpc::constants::one) == 0.0);
        REQUIRE(set.get(hmpc::detail::tag_of<double>) == 0.0);
    }

    SECTION("Two elements")
    {
        auto set = hmpc::detail::type_set<double, int>{42};
        REQUIRE(set.size == 2);
        REQUIRE(set.contains<int>());
        REQUIRE(set.contains<double>());
        REQUIRE(set.get(hmpc::constants::zero) == 42.0);
        REQUIRE(set.get(hmpc::detail::tag_of<int>) == 0);
        REQUIRE(set.get(hmpc::constants::one) == 0);
        REQUIRE(set.get(hmpc::detail::tag_of<double>) == 42.0);
    }

    SECTION("Insert elements")
    {
        auto set = hmpc::detail::type_set<int, double>{42, 11.0};
        auto plus_int = set.insert(4);
        REQUIRE(plus_int.size == 2);
        REQUIRE(std::same_as<decltype(set), decltype(plus_int)>);

        auto plus_uint = set.insert(4u);
        REQUIRE(plus_uint.size == 3);
        REQUIRE(plus_uint.get(hmpc::constants::zero) == 42);
        REQUIRE(plus_uint.get(hmpc::detail::tag_of<int>) == 42);
        REQUIRE(plus_uint.get(hmpc::constants::one) == 11.0);
        REQUIRE(plus_uint.get(hmpc::detail::tag_of<double>) == 11.0);
        REQUIRE(plus_uint.get(hmpc::size_constant_of<2>) == 4);
        REQUIRE(plus_uint.get(hmpc::detail::tag_of<unsigned>) == 4);
    }

    SECTION("Store struct with reference members")
    {
        int x = 1;
        int y = 2;
        auto set = hmpc::detail::type_set<int, struct_with_reference>{x, struct_with_reference{x, y}};
        REQUIRE(set.size == 2);

        auto plus_uint = set.insert(4u);
        y = 3;

        REQUIRE(plus_uint.size == 3);
        REQUIRE(plus_uint.get(hmpc::constants::zero) == x);
        REQUIRE(plus_uint.get(hmpc::detail::tag_of<int>) == x);
        REQUIRE(plus_uint.get(hmpc::constants::one).value == x);
        REQUIRE(plus_uint.get(hmpc::constants::one).reference == 3);
        REQUIRE(plus_uint.get(hmpc::constants::one).reference == y);
        REQUIRE(plus_uint.get(hmpc::detail::tag_of<struct_with_reference>).value == x);
        REQUIRE(plus_uint.get(hmpc::detail::tag_of<struct_with_reference>).reference == 3);
        REQUIRE(plus_uint.get(hmpc::size_constant_of<2>) == 4);
        REQUIRE(plus_uint.get(hmpc::detail::tag_of<unsigned>) == 4);
    }
}
