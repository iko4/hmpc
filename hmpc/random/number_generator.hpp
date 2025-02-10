#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/crypto/chacha.hpp>

namespace hmpc::random
{
    template<typename Engine>
    concept engine = requires(Engine engine, typename Engine::param_type param)
    {
        typename Engine::param_type;
        typename Engine::block_type;
        typename Engine::value_type;

        Engine::key_size;
        Engine::nonce_size;
        Engine::counter_size;

        { engine() } -> std::same_as<typename Engine::block_type>;

        { engine.param() } -> std::same_as<typename Engine::param_type>;
        engine.param(param);
    } and std::constructible_from<Engine, typename Engine::param_type>;

    template<typename Generator>
    concept generator = engine<typename Generator::engine_type>
        and std::same_as<typename Generator::param_type, typename Generator::engine_type::param_type>
        and std::same_as<typename Generator::value_type, typename Generator::engine_type::value_type>
        and std::constructible_from<Generator, typename Generator::param_type>
        and Generator::key_size == Generator::engine_type::key_size
        and Generator::nonce_size == Generator::engine_type::nonce_size
        and Generator::counter_size == Generator::engine_type::counter_size
        and requires(Generator generator, typename Generator::param_type param)
    {
        generator.uniform(hmpc::core::write_nullspan<typename Generator::value_type>);

        { generator.param() } -> std::same_as<typename Generator::param_type>;
        generator.param(param);
    };

    template<engine Engine = hmpc::default_random_engine>
    struct number_generator
    {
        using engine_type = Engine;
        using param_type = engine_type::param_type;
        using block_type = engine_type::block_type;
        using value_type = engine_type::value_type;

        static constexpr hmpc::size key_size = engine_type::key_size;
        static constexpr hmpc::size nonce_size = engine_type::nonce_size;
        static constexpr hmpc::size counter_size = engine_type::counter_size;

        engine_type engine;
        block_type state;
        hmpc::size block_index;

        template<typename... Args>
        constexpr number_generator(Args&&... args) HMPC_NOEXCEPT
            : engine(std::forward<Args>(args)...)
            , state(engine())
            , block_index(0)
        {
        }

        template<hmpc::write_only_limb_span Result>
        constexpr void uniform(Result result) HMPC_NOEXCEPT
        {
            hmpc::iter::for_range<result.limb_size>([&](auto i)
            {
                result.write(i, state[block_index++]);
                if (block_index == engine_type::block_size)
                {
                    state = engine();
                    block_index = 0;
                }
            });
        }

        template<hmpc::write_only_bit_span Result>
        constexpr void uniform(Result result) HMPC_NOEXCEPT
        {
            hmpc::iter::for_range<result.limb_size>([&](auto i)
            {
                result.write(i, state[block_index++], hmpc::access::normal);
                if (block_index == engine_type::block_size)
                {
                    state = engine();
                    block_index = 0;
                }
            });
        }

        constexpr param_type param() const noexcept
        {
            return engine.param();
        }

        constexpr void param(param_type param) HMPC_NOEXCEPT
        {
            engine.param(param);
            state = engine();
            block_index = 0;
        }
    };

    template<typename... Args>
    consteval auto compiletime_number_generator(Args&&... args)
    {
        return number_generator(hmpc::compiletime, std::forward<Args>(args)...);
    }
}
