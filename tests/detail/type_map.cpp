#include "catch_helpers.hpp"

#include <hmpc/constants.hpp>
#include <hmpc/detail/type_map.hpp>

template<typename... T>
using list = hmpc::detail::type_list<T...>;

TEST_CASE("Type map", "[type_map]")
{
    SECTION("Empty map")
    {
        auto map = hmpc::detail::type_map{};
        REQUIRE(map.size == 0);
        REQUIRE_FALSE(map.contains<int>());
        REQUIRE_FALSE(map.contains<double>());

        REQUIRE(map.get_or(hmpc::detail::tag_of<int>, 42) == 42);
    }

    SECTION("One element")
    {
        auto map = hmpc::detail::type_map<list<int>, list<int>>{42};
        REQUIRE(map.size == 1);
        REQUIRE(map.contains<int>());
        REQUIRE_FALSE(map.contains<double>());
        REQUIRE(map.get(hmpc::constants::zero) == 42);
        REQUIRE(map.get(hmpc::detail::tag_of<int>) == 42);

        map = std::move(map).insert_or_assign(hmpc::detail::tag_of<int>, 4);
        REQUIRE(map.get(hmpc::constants::zero) == 4);
        REQUIRE(map.get(hmpc::detail::tag_of<int>) == 4);

        auto updated_map = std::move(map).insert_or_assign(hmpc::detail::tag_of<int>, 11.0);
        REQUIRE(updated_map.size == 1);
        REQUIRE(updated_map.contains<int>());
        REQUIRE_FALSE(updated_map.contains<double>());
        REQUIRE(updated_map.get(hmpc::constants::zero) == 11.0);
        REQUIRE(updated_map.get(hmpc::detail::tag_of<int>) == 11.0);
        REQUIRE(std::same_as<decltype(updated_map.get(hmpc::constants::zero)), double&>);

        auto inserted_map = std::move(updated_map).insert_or_assign(hmpc::detail::tag_of<double>, 1.1);
        REQUIRE(inserted_map.size == 2);
        REQUIRE(inserted_map.contains<int>());
        REQUIRE(inserted_map.contains<double>());
        REQUIRE(inserted_map.get(hmpc::constants::zero) == 11.0);
        REQUIRE(inserted_map.get(hmpc::detail::tag_of<int>) == 11.0);
        REQUIRE(inserted_map.get(hmpc::constants::one) == 1.1);
        REQUIRE(inserted_map.get(hmpc::detail::tag_of<double>) == 1.1);
        REQUIRE(std::same_as<decltype(inserted_map.get(hmpc::constants::zero)), double&>);
        REQUIRE(std::same_as<decltype(inserted_map.get(hmpc::constants::one)), double&>);
    }

    SECTION("Update in middle")
    {
        auto map = hmpc::detail::type_map<list<int, char, double>, list<int, int, int>>{};

        auto updated_map = std::move(map).insert_or_assign(hmpc::detail::tag_of<char>, 11.0);
        REQUIRE(std::same_as<decltype(updated_map), hmpc::detail::type_map<list<int, char, double>, list<int, double, int>>>);
        REQUIRE(updated_map.get(hmpc::constants::zero) == 0);
        REQUIRE(updated_map.get(hmpc::constants::one) == 11.0);
        REQUIRE(updated_map.get(hmpc::constants::two) == 0);
    }
}
