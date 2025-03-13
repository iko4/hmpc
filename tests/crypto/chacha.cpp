#include "catch_helpers.hpp"

#include <hmpc/crypto/chacha.hpp>

// Test vectors from [RFC 7539](https://www.rfc-editor.org/rfc/rfc7539) / [RFC 8439](https://www.rfc-editor.org/rfc/rfc8439)

TEST_CASE("ChaCha 20", "[crypto][chacha]")
{
    SECTION("Quarter round")
    {
        hmpc::core::uint32 a = 0x1111'1111;
        hmpc::core::uint32 b = 0x0102'0304;
        hmpc::core::uint32 c = 0x9b8d'6f43;
        hmpc::core::uint32 d = 0x0123'4567;

        hmpc::crypto::detail::chacha_quarter_round(a, b, c, d);

        REQUIRE(a == 0xea2a'92f4);
        REQUIRE(b == 0xcb1c'f8ce);
        REQUIRE(c == 0x4581'472e);
        REQUIRE(d == 0x5881'c4bb);
    }

    SECTION("Block function")
    {
        hmpc::core::uint32 const key[] = {0x0302'0100, 0x0706'0504, 0x0b0a'0908, 0x0f0e'0d0c, 0x1312'1110, 0x1716'1514, 0x1b1a'1918, 0x1f1e'1d1c};
        hmpc::core::uint32 const nonce[] = {0x0900'0000, 0x4a00'0000, 0x0000'0000};
        hmpc::core::uint32 const counter[] = {0x1};

        hmpc::crypto::chacha20 chacha({key, nonce, counter});

        REQUIRE(chacha.state[0] == 0x6170'7865);
        REQUIRE(chacha.state[1] == 0x3320'646e);
        REQUIRE(chacha.state[2] == 0x7962'2d32);
        REQUIRE(chacha.state[3] == 0x6b20'6574);
        REQUIRE(chacha.state[4] == 0x0302'0100);
        REQUIRE(chacha.state[5] == 0x0706'0504);
        REQUIRE(chacha.state[6] == 0x0b0a'0908);
        REQUIRE(chacha.state[7] == 0x0f0e'0d0c);
        REQUIRE(chacha.state[8] == 0x1312'1110);
        REQUIRE(chacha.state[9] == 0x1716'1514);
        REQUIRE(chacha.state[10] == 0x1b1a'1918);
        REQUIRE(chacha.state[11] == 0x1f1e'1d1c);
        REQUIRE(chacha.state[12] == 0x0000'0001);
        REQUIRE(chacha.state[13] == 0x0900'0000);
        REQUIRE(chacha.state[14] == 0x4a00'0000);
        REQUIRE(chacha.state[15] == 0x0000'0000);

        auto state = chacha();
        REQUIRE(state[0] == 0xe4e7'f110);
        REQUIRE(state[1] == 0x1559'3bd1);
        REQUIRE(state[2] == 0x1fdd'0f50);
        REQUIRE(state[3] == 0xc471'20a3);
        REQUIRE(state[4] == 0xc7f4'd1c7);
        REQUIRE(state[5] == 0x0368'c033);
        REQUIRE(state[6] == 0x9aaa'2204);
        REQUIRE(state[7] == 0x4e6c'd4c3);
        REQUIRE(state[8] == 0x4664'82d2);
        REQUIRE(state[9] == 0x09aa'9f07);
        REQUIRE(state[10] == 0x05d7'c214);
        REQUIRE(state[11] == 0xa202'8bd9);
        REQUIRE(state[12] == 0xd19c'12b5);
        REQUIRE(state[13] == 0xb94e'16de);
        REQUIRE(state[14] == 0xe883'd0cb);
        REQUIRE(state[15] == 0x4e3c'50a2);

        auto param = chacha.param();

        hmpc::iter::for_each(hmpc::range(hmpc::size_constant_of<8>), [&](auto i)
        {
            REQUIRE(param.key[i] == key[i]);
        });
        REQUIRE(param.counter[hmpc::constants::zero] == counter[0] + 1);
        hmpc::iter::for_each(hmpc::range(hmpc::size_constant_of<3>), [&](auto i)
        {
            REQUIRE(param.nonce[i] == nonce[i]);
        });
    }

    SECTION("Additional test vectors for the block function")
    {
        hmpc::core::uint32 key[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
        hmpc::core::uint32 nonce[] = {0x0, 0x0, 0x0};
        hmpc::core::uint32 counter[] = {0x0};

        hmpc::crypto::chacha20 chacha({key, nonce, counter});

        // Test Vector #1
        auto state = chacha();
        REQUIRE(state[0] == 0xade0'b876);
        REQUIRE(state[1] == 0x903d'f1a0);
        REQUIRE(state[2] == 0xe56a'5d40);
        REQUIRE(state[3] == 0x28bd'8653);
        REQUIRE(state[4] == 0xb819'd2bd);
        REQUIRE(state[5] == 0x1aed'8da0);
        REQUIRE(state[6] == 0xccef'36a8);
        REQUIRE(state[7] == 0xc70d'778b);
        REQUIRE(state[8] == 0x7c59'41da);
        REQUIRE(state[9] == 0x8d48'5751);
        REQUIRE(state[10] == 0x3fe0'2477);
        REQUIRE(state[11] == 0x374a'd8b8);
        REQUIRE(state[12] == 0xf4b8'436a);
        REQUIRE(state[13] == 0x1ca1'1815);
        REQUIRE(state[14] == 0x69b6'87c3);
        REQUIRE(state[15] == 0x8665'eeb2);

        // Test Vector #2
        state = chacha();
        REQUIRE(state[0] == 0xbee7'079f);
        REQUIRE(state[1] == 0x7a38'5155);
        REQUIRE(state[2] == 0x7c97'ba98);
        REQUIRE(state[3] == 0x0d08'2d73);
        REQUIRE(state[4] == 0xa029'0fcb);
        REQUIRE(state[5] == 0x6965'e348);
        REQUIRE(state[6] == 0x3e53'c612);
        REQUIRE(state[7] == 0xed7a'ee32);
        REQUIRE(state[8] == 0x7621'b729);
        REQUIRE(state[9] == 0x434e'e69c);
        REQUIRE(state[10] == 0xb033'71d5);
        REQUIRE(state[11] == 0xd539'd874);
        REQUIRE(state[12] == 0x281f'ed31);
        REQUIRE(state[13] == 0x45fb'0a51);
        REQUIRE(state[14] == 0x1f0a'e1ac);
        REQUIRE(state[15] == 0x6f4d'794b);

        // Test Vector #3
        counter[0] = 0x1;
        key[7] = 0x0100'0000;
        chacha.param({key, nonce, counter});

        state = chacha();
        REQUIRE(state[0] == 0x2452'eb3a);
        REQUIRE(state[1] == 0x9249'f8ec);
        REQUIRE(state[2] == 0x8d82'9d9b);
        REQUIRE(state[3] == 0xddd4'ceb1);
        REQUIRE(state[4] == 0xe825'2083);
        REQUIRE(state[5] == 0x6081'8b01);
        REQUIRE(state[6] == 0xf384'22b8);
        REQUIRE(state[7] == 0x5aaa'49c9);
        REQUIRE(state[8] == 0xbb00'ca8e);
        REQUIRE(state[9] == 0xda3b'a7b4);
        REQUIRE(state[10] == 0xc4b5'92d1);
        REQUIRE(state[11] == 0xfdf2'732f);
        REQUIRE(state[12] == 0x4436'274e);
        REQUIRE(state[13] == 0x2561'b3c8);
        REQUIRE(state[14] == 0xebdd'4aa6);
        REQUIRE(state[15] == 0xa013'6c00);

        // Test Vector #4
        counter[0] = 0x2;
        key[0] = 0x0000'ff00;
        key[7] = 0x0;
        chacha.param({key, nonce, counter});

        state = chacha();
        REQUIRE(state[0] == 0xfb4d'd572);
        REQUIRE(state[1] == 0x4bc4'2ef1);
        REQUIRE(state[2] == 0xdf92'2636);
        REQUIRE(state[3] == 0x327f'1394);
        REQUIRE(state[4] == 0xa78d'ea8f);
        REQUIRE(state[5] == 0x5e26'9039);
        REQUIRE(state[6] == 0xa1be'bbc1);
        REQUIRE(state[7] == 0xcaf0'9aae);
        REQUIRE(state[8] == 0xa25a'b213);
        REQUIRE(state[9] == 0x48a6'b46c);
        REQUIRE(state[10] == 0x1b9d'9bcb);
        REQUIRE(state[11] == 0x092c'5be6);
        REQUIRE(state[12] == 0x546c'a624);
        REQUIRE(state[13] == 0x1bec'45d5);
        REQUIRE(state[14] == 0x87f4'7473);
        REQUIRE(state[15] == 0x96f0'992e);

        // Test Vector #5
        counter[0] = 0x0;
        key[0] = 0x0;
        nonce[2] = 0x0200'0000;
        chacha.param({key, nonce, counter});

        state = chacha();
        REQUIRE(state[0] == 0x374d'c6c2);
        REQUIRE(state[1] == 0x3736'd58c);
        REQUIRE(state[2] == 0xb904'e24a);
        REQUIRE(state[3] == 0xcd3f'93ef);
        REQUIRE(state[4] == 0x8822'8b1a);
        REQUIRE(state[5] == 0x96a4'dfb3);
        REQUIRE(state[6] == 0x5b76'ab72);
        REQUIRE(state[7] == 0xc727'ee54);
        REQUIRE(state[8] == 0x0e0e'978a);
        REQUIRE(state[9] == 0xf314'5c95);
        REQUIRE(state[10] == 0x1b74'8ea8);
        REQUIRE(state[11] == 0xf786'c297);
        REQUIRE(state[12] == 0x99c2'8f5f);
        REQUIRE(state[13] == 0x6283'14e8);
        REQUIRE(state[14] == 0x398a'19fa);
        REQUIRE(state[15] == 0x6ded'1b53);
    }
}
