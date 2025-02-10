#pragma once

#include <hmpc/core/converting_limb_span.hpp>
#include <hmpc/core/num/bit_xor.hpp>
#include <hmpc/random/number_generator.hpp>

namespace hmpc::crypto
{
    template<hmpc::random::engine Engine = hmpc::default_random_engine>
    struct cipher
    {
        using engine_type = Engine;
        using generator_type = hmpc::random::number_generator<engine_type>;
        using param_type = generator_type::param_type;
        using block_type = generator_type::block_type;
        using value_type = generator_type::value_type;

        static constexpr hmpc::size key_size = generator_type::key_size;
        static constexpr hmpc::size nonce_size = generator_type::nonce_size;
        static constexpr hmpc::size counter_size = generator_type::counter_size;

        generator_type generator;

        constexpr cipher(param_type param) HMPC_NOEXCEPT
            : generator(param)
        {
        }

        template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span Plaintext>
            requires hmpc::same_limb_types<Result, Plaintext>
        constexpr void enc(Result result, Plaintext plaintext) HMPC_NOEXCEPT
        {
            static_assert(result.limb_size == plaintext.limb_size);
            hmpc::core::limb_array<result.limb_size, typename Result::limb_type> mask;
            generator.uniform(hmpc::core::convert_limb_span<value_type>(mask.span(hmpc::access::write)));
            hmpc::core::num::bit_xor(result, plaintext, mask.span(hmpc::access::read));
        }

        template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span Ciphertext>
            requires hmpc::same_limb_types<Result, Ciphertext>
        constexpr void dec(Result result, Ciphertext ciphertext) HMPC_NOEXCEPT
        {
            enc(result, ciphertext);
        }

        constexpr param_type param() const noexcept
        {
            return generator.param();
        }

        constexpr void param(param_type param) HMPC_NOEXCEPT
        {
            generator.param(param);
        }
    };
}
