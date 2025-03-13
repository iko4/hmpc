#pragma once

#include <hmpc/comp/vector.hpp>
#include <hmpc/config.hpp>
#include <hmpc/core/divide.hpp>
#include <hmpc/core/lcm.hpp>
#include <hmpc/core/size_limb_span.hpp>
#include <hmpc/crypto/cipher.hpp>
#include <hmpc/expr/cache.hpp>
#include <hmpc/expr/expression.hpp>
#include <hmpc/ints/num/bit_copy.hpp>

namespace hmpc::expr::crypto
{
    template<hmpc::random::engine Engine>
    struct cipher_expression
    {
        using engine_type = Engine;
        using limb_type = engine_type::value_type;

        hmpc::core::limb_span<engine_type::key_size, limb_type, hmpc::access::read_tag> key;
        hmpc::core::limb_span<engine_type::nonce_size, limb_type, hmpc::access::read_tag> nonce;
    };

    template<hmpc::random::engine Engine = hmpc::default_random_engine>
    constexpr auto cipher(hmpc::core::limb_span<Engine::key_size, typename Engine::value_type, hmpc::access::read_tag> key, hmpc::core::limb_span<Engine::nonce_size, typename Engine::value_type, hmpc::access::read_tag> nonce)
    {
        return cipher_expression<Engine>{key, nonce};
    }

    template<hmpc::random::engine Engine>
    struct cipher_storage
    {
        using engine_type = Engine;
        using cipher_type = hmpc::crypto::cipher<engine_type>;
        using param_type = cipher_type::param_type;
        using limb_type = cipher_type::value_type;

        hmpc::core::limb_array<cipher_type::key_size, limb_type> key;
        hmpc::core::limb_array<cipher_type::nonce_size, limb_type> nonce;

        constexpr cipher_storage(cipher_expression<engine_type> const& cipher)
        {
            hmpc::ints::num::bit_copy(key, cipher.key);
            hmpc::ints::num::bit_copy(nonce, cipher.nonce);
        }

        constexpr auto cipher(hmpc::size index) const
        {
            HMPC_DEVICE_ASSERT(index >= 0);
            hmpc::core::limb_array<cipher_type::counter_size, limb_type> counter;
            hmpc::ints::num::bit_copy(
                counter,
                hmpc::core::size_limb_span<limb_type>{index}
            );
            auto param = param_type{
                key.span(hmpc::access::read),
                nonce.span(hmpc::access::read),
                counter.span(hmpc::access::read)
            };
            return cipher_type{param};
        }
    };

    template<hmpc::expression E, hmpc::random::engine Engine = hmpc::default_random_engine, auto Tag = []{}>
    struct enc_expression : public enable_caching
    {
        using enable_caching::operator();
        using inner_type = E;
        using engine_type = Engine;

        using limb_type = engine_type::value_type;
        using value_type = limb_type;
        using element_type = limb_type;

        using inner_value_type = inner_type::value_type;
        using inner_element_type = hmpc::traits::element_type_t<inner_value_type>;
        using inner_limb_type = hmpc::traits::limb_type_t<inner_element_type>;
        static constexpr auto limb_size = hmpc::traits::limb_size<inner_value_type>{};
        using limb_vector_type = hmpc::comp::vector<limb_type, limb_size>;

        using shape_type = hmpc::traits::element_shape_t<
            limb_vector_type,
            hmpc::expr::traits::element_shape_t<inner_type>
        >;
        using element_shape_type = shape_type;

        static_assert(std::same_as<limb_type, inner_limb_type>);

        static constexpr auto block_size = engine_type::block_size;
        static constexpr auto batch_size = hmpc::core::lcm(block_size, limb_size);
        static constexpr auto elements_per_batch = hmpc::size_constant_of<batch_size / limb_size>;
        static constexpr hmpc::size blocks_per_batch = batch_size / block_size;

        using cipher_expression_type = cipher_expression<engine_type>;

        static constexpr auto arity = hmpc::size_constant_of<1>;
        using is_complex = void;

        cipher_expression_type cipher;
        inner_type inner;

        constexpr enc_expression(cipher_expression_type const& cipher, inner_type const& inner) HMPC_NOEXCEPT
            : cipher(cipher)
            , inner(inner)
        {
        }

        constexpr inner_type const& get(hmpc::size_constant<0>) const noexcept
        {
            return inner;
        }

        static constexpr hmpc::access::once_tag access(hmpc::size_constant<0>) noexcept
        {
            return {};
        }

        constexpr shape_type shape() const noexcept
        {
            return hmpc::element_shape<limb_vector_type>(hmpc::expr::element_shape(inner));
        }

        constexpr auto operator()(auto& sycl_queue, auto& get_state, auto& get_capability_data, auto& make_capabilities, auto& tensor, auto&) const HMPC_NOEXCEPT
        {
            return sycl_queue.submit([&](auto& handler)
            {
                auto state = get_state(handler).get(hmpc::constants::zero);
                auto write = hmpc::comp::device_accessor(tensor, handler, hmpc::access::discard_write);
                auto element_shape = hmpc::expr::element_shape(inner);
                auto capability_data = get_capability_data(element_shape);
                // TODO: number of blocks should be less than hmpc::core::shift_left(hmpc::constants::one, hmpc::size_constant_of<engine_type::counter_size * limb_type::bit_size>)
                auto element_count = element_shape.size();
                auto limb_count = element_count * limb_size;
                auto batch_count = hmpc::core::ceil_divide(limb_count, batch_size);
                auto storage = cipher_storage{cipher};

                handler.parallel_for(sycl::range{batch_count}, [=](hmpc::size b)
                {
                    auto cipher = storage.cipher(b * blocks_per_batch);

                    hmpc::iter::for_each(hmpc::range(elements_per_batch), [&](auto e)
                    {
                        auto i = b * elements_per_batch + e;

                        if (i < element_count) // batches might be too big -> do nothing if out of allowed range
                        {
                            auto index = [&]()
                            {
                                if constexpr (hmpc::expr::same_element_shape<inner_type>)
                                {
                                    return i;
                                }
                                else
                                {
                                    return hmpc::from_linear_index(i, element_shape);
                                }
                            }();

                            auto capabilities = make_capabilities(capability_data, index, element_shape);

                            auto element = inner_type::operator()(state, index, capabilities);

                            hmpc::core::limb_array<limb_size, limb_type> encrypted_limbs;
                            cipher.enc(encrypted_limbs.span(hmpc::access::write), element.span(hmpc::access::read));

                            hmpc::iter::for_each(hmpc::range(limb_size), [&](auto j)
                            {
                                write[i * limb_size + j] = encrypted_limbs[j];
                            });
                        }
                    });
                });
            });
        }
    };

    template<auto Tag = []{}, hmpc::random::engine Engine = hmpc::default_random_engine, hmpc::expression E>
    constexpr auto enc(cipher_expression<Engine> cipher, E expr)
    {
        return enc_expression<E, Engine, Tag>{cipher, expr};
    }

    template<hmpc::value T, hmpc::expression E, hmpc::random::engine Engine = hmpc::default_random_engine, auto Tag = []{}>
    struct dec_expression : public enable_caching
    {
        using enable_caching::operator();
        using inner_type = E;
        using engine_type = Engine;
        using value_type = T;

        using element_type = hmpc::traits::element_type_t<value_type>;
        using limb_type = hmpc::traits::limb_type_t<value_type>;
        static constexpr auto limb_size = hmpc::traits::limb_size<value_type>{};

        static_assert(std::same_as<limb_type, typename inner_type::value_type>);
        static_assert(inner_type::shape_type::rank >= 1);
        static_assert(std::same_as<typename inner_type::shape_type, hmpc::expr::traits::element_shape_t<inner_type>>);
        static_assert(inner_type::shape_type::extent(hmpc::size_constant_of<inner_type::shape_type::rank - 1>) == limb_size);
        static_assert(hmpc::detail::implies(hmpc::vector<value_type>, inner_type::shape_type::rank >= 2 and inner_type::shape_type::extent(hmpc::size_constant_of<inner_type::shape_type::rank - 2>) == hmpc::traits::vector_size_v<value_type>));

        using shape_type = std::conditional_t<
            hmpc::vector<value_type>,
            decltype(
                hmpc::squeeze(
                    hmpc::squeeze(
                        std::declval<typename inner_type::shape_type>(),
                        hmpc::constants::minus_one,
                        hmpc::force
                    ),
                    hmpc::constants::minus_one,
                    hmpc::force
                )
            ),
            decltype(
                hmpc::squeeze(
                    std::declval<typename inner_type::shape_type>(),
                    hmpc::constants::minus_one,
                    hmpc::force
                )
            )
        >;

        static constexpr auto block_size = engine_type::block_size;
        static constexpr auto batch_size = hmpc::core::lcm(block_size, limb_size);
        static constexpr auto elements_per_batch = hmpc::size_constant_of<batch_size / limb_size>;
        static constexpr hmpc::size blocks_per_batch = batch_size / block_size;

        using cipher_expression_type = cipher_expression<engine_type>;

        static constexpr auto arity = hmpc::size_constant_of<1>;
        using is_complex = void;

        cipher_expression_type cipher;
        inner_type inner;

        constexpr dec_expression(cipher_expression_type const& cipher, inner_type const& inner) HMPC_NOEXCEPT
            : cipher(cipher)
            , inner(inner)
        {
        }

        constexpr inner_type const& get(hmpc::size_constant<0>) const noexcept
        {
            return inner;
        }

        static constexpr hmpc::access::once_tag access(hmpc::size_constant<0>) noexcept
        {
            return {};
        }

        constexpr shape_type shape() const noexcept
        {
            auto element_shape = hmpc::squeeze(inner.shape(), hmpc::constants::minus_one, hmpc::force);
            if constexpr (hmpc::vector<value_type>)
            {
                return hmpc::squeeze(element_shape, hmpc::constants::minus_one, hmpc::force);
            }
            else
            {
                return element_shape;
            }
        }

        constexpr auto operator()(auto& sycl_queue, auto& get_state, auto& get_capability_data, auto& make_capabilities, auto& tensor, auto&) const HMPC_NOEXCEPT
        {
            return sycl_queue.submit([&](auto& handler)
            {
                auto state = get_state(handler).get(hmpc::constants::zero);
                auto write = hmpc::comp::device_accessor(tensor, handler, hmpc::access::discard_write);
                auto shape = hmpc::expr::element_shape(inner);
                auto capability_data = get_capability_data(shape);
                // TODO: number of blocks should be less than hmpc::core::shift_left(hmpc::constants::one, hmpc::size_constant_of<engine_type::counter_size * limb_type::bit_size>)
                auto limb_count = shape.size();
                auto batch_count = hmpc::core::ceil_divide(limb_count, batch_size);
                auto storage = cipher_storage{cipher};

                handler.parallel_for(sycl::range{batch_count}, [=](hmpc::size b)
                {
                    auto cipher = storage.cipher(b * blocks_per_batch);

                    hmpc::iter::for_each(hmpc::range(elements_per_batch), [&](auto e)
                    {
                        auto i = b * elements_per_batch + e;

                        hmpc::core::limb_array<limb_size, limb_type> encrypted_limbs;
                        hmpc::iter::for_each(hmpc::range(limb_size), [&](auto l)
                        {
                            auto index = [&]()
                            {
                                if constexpr (hmpc::expr::same_element_shape<inner_type>)
                                {
                                    return i * limb_size + l;
                                }
                                else
                                {
                                    return hmpc::from_linear_index(i * limb_size + l, shape);
                                }
                            }();

                            auto capabilities = make_capabilities(capability_data, index, shape);

                            encrypted_limbs[l] = inner_type::operator()(state, index, capabilities);
                        });

                        element_type element;
                        cipher.dec(element.span(hmpc::access::write), encrypted_limbs.span(hmpc::access::read));
                        write[i] = element;
                    });
                });
            });
        }
    };

    template<hmpc::value T, auto Tag = []{}, hmpc::random::engine Engine = hmpc::default_random_engine, hmpc::expression E>
    constexpr auto dec(cipher_expression<Engine> cipher, E expr)
    {
        return dec_expression<T, E, Engine, Tag>{cipher, expr};
    }
}
