#include "catch_helpers.hpp"

#include <hmpc/core/uint.hpp>
#include <hmpc/net/ffi.hpp>

TEST_CASE("Network: Datatypes", "[net][ffi]")
{
    using hmpc::net::traits::message_datatype_of;

    CHECK(message_datatype_of<hmpc::core::uint1>::is_little_endian == true);
    CHECK(message_datatype_of<hmpc::core::uint1>::value == 0b0000'0001);

    CHECK(message_datatype_of<hmpc::core::uint8>::is_little_endian == true);
    CHECK(message_datatype_of<hmpc::core::uint8>::value == 0b0000'1001);

    if (message_datatype_of<hmpc::core::uint16>::is_little_endian)
    {
        CHECK(message_datatype_of<hmpc::core::uint16>::value == 0b0001'0001);
    }
    else
    {
        CHECK(message_datatype_of<hmpc::core::uint16>::value == 0b0001'0000);
    }

    if (message_datatype_of<hmpc::core::uint32>::is_little_endian)
    {
        CHECK(message_datatype_of<hmpc::core::uint32>::value == 0b0010'0001);
    }
    else
    {
        CHECK(message_datatype_of<hmpc::core::uint32>::value == 0b0010'0000);
    }

    if (message_datatype_of<hmpc::core::uint64>::is_little_endian)
    {
        CHECK(message_datatype_of<hmpc::core::uint64>::value == 0b0100'0001);
    }
    else
    {
        CHECK(message_datatype_of<hmpc::core::uint64>::value == 0b0100'0000);
    }
}
