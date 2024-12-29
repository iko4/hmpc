#include "catch_helpers.hpp"

#include <hmpc/signedness.hpp>

TEST_CASE("Signedness", "[signedness]")
{
    REQUIRE(hmpc::without_sign == hmpc::signedness::unsigned_);
    REQUIRE(hmpc::with_sign == hmpc::signedness::signed_);
    REQUIRE(std::same_as<std::underlying_type_t<hmpc::signedness>, bool>);

    CHECK(hmpc::signedness{false} == hmpc::without_sign);
    CHECK(hmpc::signedness{true} == hmpc::with_sign);
    CHECK_FALSE(static_cast<bool>(hmpc::without_sign));
    CHECK(static_cast<bool>(hmpc::with_sign));

    CHECK_FALSE(hmpc::is_signed(hmpc::without_sign));
    CHECK(hmpc::is_signed(hmpc::with_sign));

    CHECK(hmpc::is_unsigned(hmpc::without_sign));
    CHECK_FALSE(hmpc::is_unsigned(hmpc::with_sign));

    CHECK(hmpc::common_signedness(hmpc::without_sign, hmpc::without_sign) == hmpc::without_sign);
    CHECK(hmpc::common_signedness(hmpc::with_sign, hmpc::without_sign) == hmpc::with_sign);
    CHECK(hmpc::common_signedness(hmpc::without_sign, hmpc::with_sign) == hmpc::with_sign);
    CHECK(hmpc::common_signedness(hmpc::with_sign, hmpc::with_sign) == hmpc::with_sign);
}
