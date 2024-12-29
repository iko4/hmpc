#pragma once

#include <hmpc/constants.hpp>
#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/limb_array.hpp>
#include <hmpc/core/uint.hpp>
#include <hmpc/ints/num/add.hpp>
#include <hmpc/iter/for_packed_range.hpp>
#include <hmpc/iter/for_range.hpp>

namespace hmpc::crypto::detail
{
    constexpr void chacha_quarter_round(hmpc::core::uint32& a, hmpc::core::uint32& b, hmpc::core::uint32& c, hmpc::core::uint32& d) HMPC_NOEXCEPT
    {
        a += b;
        d ^= a;
        d <<= hmpc::constant_of<hmpc::rotate{16}>;

        c += d;
        b ^= c;
        b <<= hmpc::constant_of<hmpc::rotate{12}>;

        a += b;
        d ^= a;
        d <<= hmpc::constant_of<hmpc::rotate{8}>;

        c += d;
        b ^= c;
        b <<= hmpc::constant_of<hmpc::rotate{7}>;
    }

    constexpr void chacha_double_round(hmpc::core::limb_span<16, hmpc::core::uint32, hmpc::access::read_write_tag> state) HMPC_NOEXCEPT
    {
        hmpc::iter::for_range<hmpc::size{4}>([&](auto i)
        {
            hmpc::iter::for_packed_range<hmpc::size{4}>([&](auto... j)
            {
                chacha_quarter_round(state[hmpc::size_constant_of<i + 4 * j>]...);
            });
        });

        chacha_quarter_round(state[hmpc::size_constant_of<0>], state[hmpc::size_constant_of<5>], state[hmpc::size_constant_of<10>], state[hmpc::size_constant_of<15>]);
        chacha_quarter_round(state[hmpc::size_constant_of<1>], state[hmpc::size_constant_of<6>], state[hmpc::size_constant_of<11>], state[hmpc::size_constant_of<12>]);
        chacha_quarter_round(state[hmpc::size_constant_of<2>], state[hmpc::size_constant_of<7>], state[hmpc::size_constant_of<8>], state[hmpc::size_constant_of<13>]);
        chacha_quarter_round(state[hmpc::size_constant_of<3>], state[hmpc::size_constant_of<4>], state[hmpc::size_constant_of<9>], state[hmpc::size_constant_of<14>]);
    }
}

namespace hmpc::crypto
{
    template<hmpc::size NonceSize, hmpc::size CounterSize>
    struct chacha_param
    {
        static_assert(CounterSize > 0);
        static_assert(NonceSize > 0);
        static_assert(CounterSize + NonceSize == 4);

        static constexpr hmpc::size key_size = 8;
        static constexpr hmpc::size counter_size = CounterSize;
        static constexpr hmpc::size nonce_size = NonceSize;

        using value_type = hmpc::core::uint32;

        hmpc::core::limb_span<key_size, value_type, hmpc::access::read_tag> key;
        hmpc::core::limb_span<nonce_size, value_type, hmpc::access::read_tag> nonce;
        hmpc::core::limb_span<counter_size, value_type, hmpc::access::read_tag> counter;
    };

    template<hmpc::size Rounds, hmpc::size NonceSize, hmpc::size CounterSize>
    struct chacha
    {
        static_assert(Rounds >= 8);
        static_assert(Rounds % 2 == 0);

        static constexpr hmpc::size key_size = 8;
        static constexpr hmpc::size counter_size = CounterSize;
        static constexpr hmpc::size nonce_size = NonceSize;
        static constexpr hmpc::size constant_size = 4;
        static constexpr hmpc::size block_size = 16;

        using param_type = chacha_param<nonce_size, counter_size>;
        using value_type = hmpc::core::uint32;
        using block_type = hmpc::core::limb_array<block_size, value_type>;
        block_type state;

        consteval chacha(hmpc::compiletime_tag)
            : state{0x6170'7865, 0x3320'646e, 0x7962'2d32, 0x6b20'6574}
        {
        }

        constexpr chacha(param_type param) HMPC_NOEXCEPT
            : state
            {
                hmpc::iter::for_packed_range<key_size>([&](auto... i)
                {
                    return hmpc::iter::for_packed_range<counter_size>([&](auto... j)
                    {
                        return hmpc::iter::for_packed_range<nonce_size>([&](auto... k)
                        {
                            return block_type
                            {
                                0x6170'7865,
                                0x3320'646e,
                                0x7962'2d32,
                                0x6b20'6574,
                                param.key[i]...,
                                param.counter[j]...,
                                param.nonce[k]...
                            };
                        });
                    });
                })
            }
        {
        }

        constexpr param_type param() const noexcept
        {
            auto span = state.span(hmpc::access::read);
            return param_type{
                span.subspan<constant_size, key_size>(),
                span.subspan<constant_size + key_size + counter_size, nonce_size>(),
                span.subspan<constant_size + key_size, counter_size>()
            };
        }

        constexpr void param(param_type param) HMPC_NOEXCEPT
        {
            // TODO: replace with span-based copies
            hmpc::iter::for_range<key_size>([&](auto i)
            {
                state[i + constant_size] = param.key[i];
            });
            hmpc::iter::for_range<counter_size>([&](auto i)
            {
                state[i + constant_size + key_size] = param.counter[i];
            });
            hmpc::iter::for_range<nonce_size>([&](auto i)
            {
                state[i + constant_size + key_size + counter_size] = param.nonce[i];
            });
        }

        constexpr block_type operator()() HMPC_NOEXCEPT
        {
            auto result = state;
            hmpc::iter::for_range<Rounds / 2>([&](auto)
            {
                hmpc::crypto::detail::chacha_double_round(result.span());
            });

            hmpc::iter::for_range<block_size>([&](auto i)
            {
                result[i] += state[i];
            });

            auto counter = state.span().subspan<constant_size + key_size, counter_size>();
            hmpc::ints::num::add(
                counter,
                counter,
                hmpc::core::nullspan<value_type>,
                hmpc::constants::bit::one
            );

            return result;
        }
    };
}
