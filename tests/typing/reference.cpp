#include "catch_helpers.hpp"

#include <hmpc/typing/reference.hpp>

TEST_CASE("References", "[typing][reference]")
{
    STATIC_REQUIRE_FALSE(hmpc::typing::reference<int>);
    STATIC_REQUIRE_FALSE(hmpc::typing::reference<int const>);
    STATIC_REQUIRE(hmpc::typing::reference<int&>);
    STATIC_REQUIRE(hmpc::typing::reference<int const&>);
    STATIC_REQUIRE(hmpc::typing::reference<int&&>);
    STATIC_REQUIRE(hmpc::typing::reference<int const&&>);

    STATIC_REQUIRE_FALSE(hmpc::typing::lvalue_reference<int>);
    STATIC_REQUIRE_FALSE(hmpc::typing::lvalue_reference<int const>);
    STATIC_REQUIRE(hmpc::typing::lvalue_reference<int&>);
    STATIC_REQUIRE(hmpc::typing::lvalue_reference<int const&>);
    STATIC_REQUIRE_FALSE(hmpc::typing::lvalue_reference<int&&>);
    STATIC_REQUIRE_FALSE(hmpc::typing::lvalue_reference<int const&&>);

    STATIC_REQUIRE_FALSE(hmpc::typing::rvalue_reference<int>);
    STATIC_REQUIRE_FALSE(hmpc::typing::rvalue_reference<int const>);
    STATIC_REQUIRE_FALSE(hmpc::typing::rvalue_reference<int&>);
    STATIC_REQUIRE_FALSE(hmpc::typing::rvalue_reference<int const&>);
    STATIC_REQUIRE(hmpc::typing::rvalue_reference<int&&>);
    STATIC_REQUIRE(hmpc::typing::rvalue_reference<int const&&>);

    STATIC_REQUIRE_FALSE(hmpc::typing::universal_reference_to_lvalue<int>);
    STATIC_REQUIRE_FALSE(hmpc::typing::universal_reference_to_lvalue<int const>);
    STATIC_REQUIRE(hmpc::typing::universal_reference_to_lvalue<int&>);
    STATIC_REQUIRE(hmpc::typing::universal_reference_to_lvalue<int const&>);
    STATIC_REQUIRE_FALSE(hmpc::typing::universal_reference_to_lvalue<int&&>);
    STATIC_REQUIRE_FALSE(hmpc::typing::universal_reference_to_lvalue<int const&&>);

    STATIC_REQUIRE(hmpc::typing::universal_reference_to_rvalue<int>);
    STATIC_REQUIRE(hmpc::typing::universal_reference_to_rvalue<int const>);
    STATIC_REQUIRE_FALSE(hmpc::typing::universal_reference_to_rvalue<int&>);
    STATIC_REQUIRE_FALSE(hmpc::typing::universal_reference_to_rvalue<int const&>);
    STATIC_REQUIRE(hmpc::typing::universal_reference_to_rvalue<int&&>);
    STATIC_REQUIRE(hmpc::typing::universal_reference_to_rvalue<int const&&>);
}
