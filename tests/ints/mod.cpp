#include "catch_helpers.hpp"

#include <hmpc/core/uint.hpp>
#include <hmpc/ints/bigint.hpp>
#include <hmpc/ints/mod.hpp>
#include <hmpc/ints/literals.hpp>

TEST_CASE("Integers modulo")
{
    using namespace hmpc::ints::literals;

    constexpr auto p = 0x2faeadbe7a0195c011ac195ad10269830e8001_int; // 1'063'351'684'119'354'455'646'439'919'975'158'459'206'107'137

    using mod_p = hmpc::ints::mod<p>;
    using integer = mod_p::unsigned_type;
    using limb = mod_p::limb_type;

    REQUIRE(mod_p::reduced_auxiliary_modulus == 0x14777bad2b7e4321264fea92350db2982cfaa2_int); // 456'423'350'909'896'145'476'382'670'415'296'706'741'336'738

    auto zero = mod_p(0_int);
    REQUIRE(hmpc::ints::num::equal_to(zero, hmpc::core::nullspan<limb>));
    auto one = mod_p(1_int);
    REQUIRE(hmpc::ints::num::equal_to(one, mod_p::reduced_auxiliary_modulus));

    hmpc::iter::for_range<hmpc::size{10}>([&](auto i)
    {
        REQUIRE(pow(zero, i) == (i == 0 ? one : zero));
        REQUIRE(pow(one, i) == one);
    });

    REQUIRE(static_cast<integer>(zero) == hmpc::ints::zero<limb>);
    REQUIRE(static_cast<integer>(one) == hmpc::ints::one<limb>);

    auto fivetwelve = mod_p(512_int);
    auto fivetwelve_montgomery = 0x2488b8649d2b26fd819b73b94e54ed3c8dc325_int; // 814'736'843'728'200'697'337'584'778'072'211'285'426'946'853
    REQUIRE(hmpc::ints::num::equal_to(fivetwelve, fivetwelve_montgomery));
    REQUIRE(static_cast<integer>(fivetwelve) == 0x200_int); // 512
    REQUIRE(static_cast<integer>(pow(fivetwelve, hmpc::size_constant_of<1>)) == 0x200_int); // 512
    REQUIRE(static_cast<integer>(pow(fivetwelve, hmpc::size_constant_of<2>)) == 0x40000_int); // 262'144
    REQUIRE(static_cast<integer>(pow(fivetwelve, hmpc::size_constant_of<3>)) == 0x8000000_int); // 134'217'728
    REQUIRE(static_cast<integer>(pow(fivetwelve, hmpc::size_constant_of<4>)) == 0x1000000000_int); // 68'719'476'736
    REQUIRE(static_cast<integer>(pow(fivetwelve, hmpc::size_constant_of<5>)) == 0x200000000000_int); // 35'184'372'088'832

    SECTION("Constructors")
    {
        SECTION("Unsigned")
        {
            REQUIRE(static_cast<integer>(mod_p(p)) == hmpc::ints::zero<limb>);
            REQUIRE(static_cast<integer>(mod_p(0x2faeadbe7a0195c011ac195ad10269830e8002_int)) == hmpc::ints::one<limb>); // 1'063'351'684'119'354'455'646'439'919'975'158'459'206'107'138
            REQUIRE(static_cast<integer>(mod_p(0x2faeadbe7a0195c011ac195ad10269830e80010000000000000000000000000000002a_int)) == 0x2a_int); // 361'839'827'941'500'026'654'082'605'571'947'099'827'141'228'445'934'994'300'899'793'488'879'775'214'806'761'514 == 42
        }
        SECTION("Signed")
        {
            REQUIRE(static_cast<integer>(mod_p(-0x1_int)) == 0x2faeadbe7a0195c011ac195ad10269830e8000_int); // -1 == 1'063'351'684'119'354'455'646'439'919'975'158'459'206'107'136
            REQUIRE(static_cast<integer>(mod_p(-0x2faeadbe7a0195c011ac195ad10269830e80010000000000000000000000000000002a_int)) == 0x2faeadbe7a0195c011ac195ad10269830e7fd7_int); // 361'839'827'941'500'026'654'082'605'571'947'099'827'141'228'445'934'994'300'899'793'488'879'775'214'806'761'514 == 1'063'351'684'119'354'455'646'439'919'975'158'459'206'107'095
        }
        constexpr auto q = 99_int;
        using mod_q = hmpc::ints::mod<q>;
        using integer_from_q = mod_q::unsigned_type;
        SECTION("Modulo")
        {
            for (limb i = 0; i <= 49; i += 1)
            {
                auto n = hmpc::ints::ubigint<7>{i};
                REQUIRE(static_cast<integer>(mod_p(mod_q(n))) == n);
                REQUIRE(static_cast<integer_from_q>(mod_q(mod_p(n))) == n);

                if (i != 0)
                {
                    REQUIRE(static_cast<integer>(mod_p(mod_q(-n))) == p - n);
                    REQUIRE(static_cast<integer_from_q>(mod_q(mod_p(-n))) == q - n);
                }
                else
                {
                    REQUIRE(static_cast<integer>(mod_p(mod_q(-n))) == n);
                    REQUIRE(static_cast<integer_from_q>(mod_q(mod_p(-n))) == n);
                }
            }
        }
    }

    SECTION("Format")
    {
        REQUIRE(HMPC_FMTLIB::format("{}", zero) == "0x0000000000000000000000000000000000000000");
        REQUIRE(HMPC_FMTLIB::format("{}", one) == "0x0000000000000000000000000000000000000001");
        REQUIRE(HMPC_FMTLIB::format("{}", fivetwelve) == "0x0000000000000000000000000000000000000200");
    }
}
