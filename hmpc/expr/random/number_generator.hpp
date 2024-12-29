#pragma once

#include <hmpc/shape.hpp>
#include <hmpc/core/size_limb_span.hpp>
#include <hmpc/ints/num/bit_copy.hpp>

namespace hmpc::expr::random
{
    template<auto Tag = []{}>
    struct use_number_generator
    {
    };

    template<hmpc::random::engine Engine>
    struct number_generator_expression
    {
        using engine_type = Engine;
        using limb_type = engine_type::value_type;

        hmpc::core::limb_span<engine_type::key_size, limb_type, hmpc::access::read_tag> key;
        hmpc::size offset;
    };

    template<hmpc::random::engine Engine = hmpc::default_random_engine, typename Index, typename Shape>
    constexpr auto number_generator(hmpc::core::limb_span<Engine::key_size, typename Engine::value_type, hmpc::access::read_tag> key, Index const& offset_index, Shape const& offset_shape)
    {
        auto index = hmpc::to_linear_index(offset_index, offset_shape);
        return number_generator_expression<Engine>{key, index};
    }

    template<hmpc::random::engine Engine, typename Shape>
    struct number_generator_storage
    {
        using engine_type = Engine;
        using generator_type = hmpc::random::number_generator<engine_type>;
        using param_type = generator_type::param_type;
        using limb_type = generator_type::value_type;
        using shape_type = Shape;

        hmpc::core::limb_array<generator_type::key_size, limb_type> key;
        shape_type shape;
        hmpc::size offset;

        constexpr number_generator_storage(number_generator_expression<engine_type> const& rng, shape_type const& shape)
            : shape(shape)
            , offset(rng.offset)
        {
            hmpc::ints::num::bit_copy(key, rng.key);
        }

        constexpr auto generator(hmpc::size index) const
        {
            HMPC_DEVICE_ASSERT(index >= 0);
            HMPC_DEVICE_ASSERT(index < shape.size());
            hmpc::core::limb_array<generator_type::nonce_size, limb_type> nonce;
            hmpc::ints::num::bit_copy(
                nonce,
                hmpc::core::size_limb_span<limb_type>{index + shape.size() * offset}
            );
            hmpc::core::limb_array<generator_type::counter_size, limb_type> counter = {};
            auto param = param_type{
                key.span(hmpc::access::read),
                nonce.span(hmpc::access::read),
                counter.span(hmpc::access::read)
            };
            return generator_type{param};
        }

        constexpr auto generator(hmpc::mdindex_for<shape_type> auto const& index) const
        {
            hmpc::size i = hmpc::to_linear_index(index, shape);
            return generator(i);
        }
    };
}
