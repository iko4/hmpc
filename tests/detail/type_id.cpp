#include "catch_helpers.hpp"

#include <hmpc/detail/type_id.hpp>

struct my_struct
{
    int dummy;
};

struct undefined;

TEST_CASE("Type id", "[type_id]")
{
    CHECK(hmpc::detail::type_id_of<int>() == hmpc::detail::type_id_of<int>());
    CHECK(hmpc::detail::type_id_of<int>() == hmpc::detail::type_id_of<int&>());
    CHECK(hmpc::detail::type_id_of<int>() == hmpc::detail::type_id_of<int&&>());
    CHECK(hmpc::detail::type_id_of<int>() == hmpc::detail::type_id_of<int const>());
    CHECK(hmpc::detail::type_id_of<int>() == hmpc::detail::type_id_of<int const&>());
    CHECK(hmpc::detail::type_id_of<int>() == hmpc::detail::type_id_of<int const&&>());

    auto id_of_int = hmpc::detail::type_id<int>::value();
    CHECK(id_of_int == hmpc::detail::type_id_of<int>());

    using my_int = int;
    CHECK(hmpc::detail::type_id_of<int>() == hmpc::detail::type_id_of<my_int>());

    CHECK(hmpc::detail::type_id_of<int>() != hmpc::detail::type_id_of<unsigned int>());
    CHECK(hmpc::detail::type_id_of<int>() != hmpc::detail::type_id_of<double>());
    CHECK(hmpc::detail::type_id_of<int>() != hmpc::detail::type_id_of<float>());
    CHECK(hmpc::detail::type_id_of<int>() != hmpc::detail::type_id_of<my_struct>());
    CHECK(hmpc::detail::type_id_of<int>() != hmpc::detail::type_id_of<undefined>());

    CHECK(hmpc::detail::type_id_of<char>() != hmpc::detail::type_id_of<signed char>());
    CHECK(hmpc::detail::type_id_of<char>() != hmpc::detail::type_id_of<unsigned char>());
    CHECK(hmpc::detail::type_id_of<signed char>() != hmpc::detail::type_id_of<unsigned char>());
    CHECK(hmpc::detail::type_id_of<char>() != hmpc::detail::type_id_of<std::byte>());
    CHECK(hmpc::detail::type_id_of<signed char>() != hmpc::detail::type_id_of<std::byte>());
    CHECK(hmpc::detail::type_id_of<unsigned char>() != hmpc::detail::type_id_of<std::byte>());
}
