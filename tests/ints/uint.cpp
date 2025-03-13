#include "catch_helpers.hpp"

#include <hmpc/core/uint.hpp>
#include <hmpc/ints/uint.hpp>

TEST_CASE("Unsigned integer constructor", "[ints][uint]")
{
    using hmpc::core::uint16;
    using hmpc::core::uint32;

    using uint100at16 = hmpc::ints::uint<100, uint16>;
    using uint100at32 = hmpc::ints::uint<100, uint32>;

    REQUIRE(uint100at16::bit_size == 100);
    REQUIRE(uint100at32::bit_size == 100);

    REQUIRE(uint100at16::limb_bit_size == 16);
    REQUIRE(uint100at32::limb_bit_size == 32);

    REQUIRE(uint100at16::limb_size == 7);
    REQUIRE(uint100at32::limb_size == 4);

    auto u16 = uint100at16{1, 2, 3, 4, 5, 6, 7};
    auto u32 = uint100at32{1, 2, 3, 4};

    auto v16 = u16.span();
    auto v32 = u32.span();

    SECTION("Reading span")
    {
        REQUIRE(v16.bit_size == 100);
        REQUIRE(v16.limb_size == 7);
        REQUIRE(v16.data[0].data == 1);
        REQUIRE(v16.data[1].data == 2);
        REQUIRE(v16.data[2].data == 3);
        REQUIRE(v16.data[3].data == 4);
        REQUIRE(v16.data[4].data == 5);
        REQUIRE(v16.data[5].data == 6);
        REQUIRE(v16.data[6].data == 7);

        REQUIRE(v32.bit_size == 100);
        REQUIRE(v32.limb_size == 4);
        REQUIRE(v32.data[0].data == 1);
        REQUIRE(v32.data[1].data == 2);
        REQUIRE(v32.data[2].data == 3);
        REQUIRE(v32.data[3].data == 4);
    }

    SECTION("Reading first bits")
    {
        auto first_v16 = v16.first_bits<42>();
        auto first_v32 = v32.first_bits<42>();

        REQUIRE(first_v16.bit_size == 42);
        REQUIRE(first_v16.limb_size == 3);
        REQUIRE(first_v16.data[0].data == 1);
        REQUIRE(first_v16.data[1].data == 2);
        REQUIRE(first_v16.data[2].data == 3);
        REQUIRE(first_v16.data.pointer == v16.data.pointer);

        REQUIRE(first_v32.bit_size == 42);
        REQUIRE(first_v32.limb_size == 2);
        REQUIRE(first_v32.data[0].data == 1);
        REQUIRE(first_v32.data[1].data == 2);
        REQUIRE(first_v32.data.pointer == v32.data.pointer);
    }

    SECTION("Reading first limbs")
    {
        auto first_v16 = v16.first_limbs<2>();
        auto first_v32 = v32.first_limbs<2>();

        REQUIRE(first_v16.bit_size == 32);
        REQUIRE(first_v16.limb_size == 2);
        REQUIRE(first_v16.data[0].data == 1);
        REQUIRE(first_v16.data[1].data == 2);
        REQUIRE(first_v16.data.pointer == v16.data.pointer);

        REQUIRE(first_v32.bit_size == 64);
        REQUIRE(first_v32.limb_size == 2);
        REQUIRE(first_v32.data[0].data == 1);
        REQUIRE(first_v32.data[1].data == 2);
        REQUIRE(first_v32.data.pointer == v32.data.pointer);
    }

    SECTION("Reading last limbs")
    {
        auto first_v16 = v16.last_limbs<2>();
        auto first_v32 = v32.last_limbs<2>();

        REQUIRE(first_v16.bit_size == 20);
        REQUIRE(first_v16.limb_size == 2);
        REQUIRE(first_v16.data[0].data == 6);
        REQUIRE(first_v16.data[1].data == 7);
        REQUIRE(first_v16.data.pointer == v16.data.pointer + 5);

        REQUIRE(first_v32.bit_size == 36);
        REQUIRE(first_v32.limb_size == 2);
        REQUIRE(first_v32.data[0].data == 3);
        REQUIRE(first_v32.data[1].data == 4);
        REQUIRE(first_v32.data.pointer == v32.data.pointer + 2);
    }

    SECTION("Reading subspan")
    {
        auto sub_v16 = v16.subspan<2>();
        auto sub_v32 = v32.subspan<2>();

        REQUIRE(sub_v16.bit_size == 68);
        REQUIRE(sub_v16.limb_size == 5);
        REQUIRE(sub_v16.data[0].data == 3);
        REQUIRE(sub_v16.data[1].data == 4);
        REQUIRE(sub_v16.data[2].data == 5);
        REQUIRE(sub_v16.data[3].data == 6);
        REQUIRE(sub_v16.data[4].data == 7);
        REQUIRE(sub_v16.data.pointer == v16.data.pointer + 2);

        REQUIRE(sub_v32.bit_size == 36);
        REQUIRE(sub_v32.limb_size == 2);
        REQUIRE(sub_v32.data[0].data == 3);
        REQUIRE(sub_v32.data[1].data == 4);
        REQUIRE(sub_v32.data.pointer == v32.data.pointer + 2);
    }
}

template<hmpc::size B>
using unsigned_integer_uint1 = hmpc::ints::uint<B, hmpc::core::uint1>;
template<hmpc::size B>
using unsigned_integer_uint8 = hmpc::ints::uint<B, hmpc::core::uint8>;
template<hmpc::size B>
using unsigned_integer_uint16 = hmpc::ints::uint<B, hmpc::core::uint16>;
template<hmpc::size B>
using unsigned_integer_uint32 = hmpc::ints::uint<B, hmpc::core::uint32>;


TEMPLATE_PRODUCT_TEST_CASE_SIG("Unsigned integer operations", "[ints][uint]", ((hmpc::size B), B), (unsigned_integer_uint1, unsigned_integer_uint8, unsigned_integer_uint16, unsigned_integer_uint32), (1, 2, 3, 4, 5, 6, 7, 8, 9, 10))
{
    using uint = TestType;

    constexpr auto bits = uint::bit_size;
    constexpr std::uint32_t max = std::uint32_t{1} << bits;
    constexpr std::uint32_t mask = max - 1;
    constexpr std::uint32_t max_test_value = 2048;
    static_assert(max <= max_test_value);

    auto to_uint32 = [](uint const& value)
    {
        unsigned_integer_uint32<bits> result;
        convert(result, value);
        return result;
    };

    for (std::uint32_t x = 0; x < max_test_value; ++x)
    {
        uint left;
        convert(left, unsigned_integer_uint32<bits>{x});

        REQUIRE((to_uint32(-left).data[0].data & mask) == ((-x) & mask));
        REQUIRE((to_uint32(-left + left).data[0].data & mask) == 0);
        REQUIRE((to_uint32(~left).data[0].data & mask) == ((~x) & mask));
        REQUIRE((to_uint32(~left ^ left).data[0].data & mask) == mask);

        hmpc::iter::for_each(hmpc::range(hmpc::size_constant_of<10>), [&](auto i)
        {
            REQUIRE((to_uint32(left << i).data[0].data & mask) == (((x & mask) << i) & mask));
            REQUIRE((to_uint32(left >> i).data[0].data & mask) == (((x & mask) >> i) & mask));
        });

        for (std::uint32_t y = 0; y < max_test_value; ++y)
        {
            uint right;
            convert(right, unsigned_integer_uint32<bits>{y});

            REQUIRE((left == right).data == ((x & mask) == (y & mask)));
            REQUIRE((left != right).data == ((x & mask) != (y & mask)));
            REQUIRE((left > right).data == ((x & mask) > (y & mask)));
            REQUIRE((left < right).data == ((x & mask) < (y & mask)));
            REQUIRE((left >= right).data == ((x & mask) >= (y & mask)));
            REQUIRE((left <= right).data == ((x & mask) <= (y & mask)));
            REQUIRE((to_uint32(left + right).data[0].data & mask) == ((x + y) & mask));
            REQUIRE((to_uint32(left - right).data[0].data & mask) == ((x - y) & mask));
            REQUIRE((to_uint32(left * right).data[0].data & mask) == ((x * y) & mask));
            REQUIRE((to_uint32(left & right).data[0].data & mask) == ((x & y) & mask));
            REQUIRE((to_uint32(left | right).data[0].data & mask) == ((x | y) & mask));
            REQUIRE((to_uint32(left ^ right).data[0].data & mask) == ((x ^ y) & mask));
        }
    }
}
