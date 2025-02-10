#pragma once

#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/device.hpp>
#include <hmpc/core/size_limb_span.hpp>
#include <hmpc/detail/hash.hpp>
#include <hmpc/detail/random.hpp>
#include <hmpc/detail/utility.hpp>
#include <hmpc/expr/cache.hpp>
#include <hmpc/expr/expression.hpp>
#include <hmpc/index.hpp>
#include <hmpc/ints/num/bit_copy.hpp>
#include <hmpc/random/number_generator.hpp>

#include <unordered_map>

namespace hmpc
{
    struct as_tuple_tag
    {
    };
    constexpr as_tuple_tag as_tuple = {};
}

namespace hmpc::comp
{
    namespace detail
    {
        constexpr auto call_packed(auto const& f, hmpc::expression auto e)
        {
            return f(e);
        }

        constexpr auto call_packed(auto const& f, hmpc::expression_tuple auto e)
        {
            using expression_type = decltype(e);
            return hmpc::iter::for_packed_range<expression_type::arity>([&](auto... i)
            {
                return f(e.get(i)...);
            });
        }

        // forward-declare to make all options available below
        constexpr auto call_packed(auto const& f, hmpc::expression_tuple auto e, auto... rest);

        constexpr auto call_packed(auto const& f, hmpc::expression auto e, auto... rest)
        {
            return call_packed(
                [&](auto... args)
                {
                    return f(e, args...);
                },
                rest...
            );
        }

        constexpr auto call_packed(auto const& f, hmpc::expression_tuple auto e, auto... rest)
        {
            using expression_type = decltype(e);
            return call_packed(
                [&](auto... args)
                {
                    return hmpc::iter::for_packed_range<expression_type::arity>([&](auto... i)
                    {
                        return f(e.get(i)..., args...);
                    });
                },
                rest...
            );
        }
    }


    struct tensor_lookup_key_view
    {
        hmpc::size type_id;
        hmpc::size limb_bit_size;
        hmpc::size limb_size;
        hmpc::size vector_size;
        hmpc::size size;
        std::string_view tag;
    };

    struct tensor_lookup_key
    {
        hmpc::size type_id;
        hmpc::size limb_bit_size;
        hmpc::size limb_size;
        hmpc::size vector_size;
        hmpc::size size;
        std::string tag;

        tensor_lookup_key() HMPC_NOEXCEPT = default;
        tensor_lookup_key(tensor_lookup_key_view other) HMPC_NOEXCEPT
            : type_id(other.type_id)
            , limb_bit_size(other.limb_bit_size)
            , limb_size(other.limb_size)
            , vector_size(other.vector_size)
            , size(other.size)
            , tag(other.tag)
        {
        }

        struct equal
        {
            using is_transparent = void;

            static constexpr bool operator()(auto const& left, auto const& right) noexcept
            {
                return left.type_id == right.type_id
                    and left.limb_bit_size == right.limb_bit_size
                    and left.limb_size == right.limb_size
                    and left.vector_size == right.vector_size
                    and left.size == right.size
                    and left.tag == right.tag;
            }
        };

        struct hash
        {
            using is_transparent = void;

            static constexpr std::size_t operator()(auto const& key) noexcept
            {
                auto size_hasher = std::hash<std::size_t>{};
                auto string_hasher = std::hash<std::string_view>{};

                auto type_id = size_hasher(key.type_id);
                auto limb_bit_size = size_hasher(key.limb_bit_size);
                auto limb_size = size_hasher(key.limb_size);
                auto vector_size = size_hasher(key.vector_size);
                auto size = size_hasher(key.size);
                auto tag = string_hasher(key.tag);

                return hmpc::detail::combine_hashes(
                    type_id,
                    limb_bit_size,
                    limb_size,
                    vector_size,
                    size,
                    tag
                );
            }
        };
    };

    struct tensor_lookup_value_base
    {
        virtual ~tensor_lookup_value_base() = default;
    };

    template<typename T, hmpc::size... Dimensions>
    struct tensor_lookup_value : public tensor_lookup_value_base
    {
        hmpc::comp::tensor<T, Dimensions...> tensor;

        constexpr tensor_lookup_value(hmpc::comp::tensor<T, Dimensions...>&& tensor) HMPC_NOEXCEPT
            : tensor(std::move(tensor))
        {
        }

        virtual ~tensor_lookup_value() override = default;
    };

    template<typename RandomNumberGenerator = hmpc::random::number_generator<>>
    struct queue
    {
        using queue_type = sycl::queue;
        using random_number_generator_type = RandomNumberGenerator;
        using random_number_generator_limb_type = random_number_generator_type::value_type;

        queue_type sycl_queue;
        std::unordered_map<tensor_lookup_key, std::unique_ptr<tensor_lookup_value_base>, tensor_lookup_key::hash, tensor_lookup_key::equal> extra_tensors;

        struct random_number_generator_state_type
        {
            hmpc::core::limb_array<random_number_generator_type::key_size, random_number_generator_limb_type> key;
            hmpc::core::limb_array<random_number_generator_type::nonce_size, random_number_generator_limb_type> nonce;
        };
        random_number_generator_state_type random_number_generator_state;

        template<typename Tag>
        struct capability_type;

        template<>
        struct capability_type<hmpc::expr::capabilities::random_number_generator_tag>
        {
            hmpc::core::limb_array<random_number_generator_type::counter_size, random_number_generator_limb_type> counter;
            random_number_generator_type random_number_generator;

            constexpr auto params_from(auto const& data, hmpc::size index) noexcept
            {
                auto const& [key, nonce] = data.get(hmpc::detail::tag_of<hmpc::expr::capabilities::random_number_generator_tag>);

                using param_type = random_number_generator_type::param_type;
                using limb_type = random_number_generator_limb_type;

                hmpc::ints::num::bit_copy(counter, hmpc::core::size_limb_span<limb_type>{index});

                return param_type{key.span(hmpc::access::read), nonce.span(hmpc::access::read), counter.span(hmpc::access::read)};
            }

            template<hmpc::size... Dimensions>
            constexpr capability_type(auto const& data, hmpc::index<Dimensions...> const& index, auto const& shape) noexcept
                : capability_type(data, hmpc::to_linear_index(index, shape), shape)
            {
            }

            constexpr capability_type(auto const& data, hmpc::size index, auto const&) noexcept
                : counter{}, random_number_generator(params_from(data, index))
            {
            }

            constexpr auto& get(hmpc::expr::capabilities::random_number_generator_tag) noexcept
            {
                return random_number_generator;
            }
        };

        template<typename... Tags>
        struct capabilities_type : public capability_type<Tags>...
        {
            constexpr capabilities_type(auto const& data, auto const& index, auto const& shape) noexcept
                : capability_type<Tags>::capability_type(data, index, shape)...
            {
            }
        };

        queue(queue_type queue)
            : sycl_queue(queue), random_number_generator_state{}
        {
            hmpc::detail::fill_random(random_number_generator_state.key);
        }

        queue(queue_type queue, std::span<random_number_generator_limb_type const, random_number_generator_type::key_size> key)
            : sycl_queue(queue), random_number_generator_state{}
        {
            std::ranges::copy(key, random_number_generator_state.key);
        }

        template<typename CapabilityData>
        constexpr auto add_capability_data(CapabilityData&& data, hmpc::detail::type_tag<hmpc::expr::capabilities::random_number_generator_tag> tag, auto const& shape) HMPC_HOST_NOEXCEPT
        {
            static_assert(not CapabilityData::contains(tag));
            auto new_data = std::forward<CapabilityData>(data).insert(tag, random_number_generator_state);
            auto nonce = random_number_generator_state.nonce.span();
            hmpc::ints::num::add(
                nonce,
                nonce,
                hmpc::core::size_limb_span<random_number_generator_limb_type>{shape.size()}
            );
            return new_data;
        }

        template<hmpc::expression E>
        static constexpr decltype(auto) add_capabilities(auto&& capabilities, E) noexcept
        {
            if constexpr (hmpc::expression_with_capabilities<E>)
            {
                using capabilities_type = E::capabilities;
                return hmpc::iter::scan_range<capabilities_type::size>([&](auto i, auto&& capabilities) -> decltype(auto)
                {
                    return std::forward<decltype(capabilities)>(capabilities).insert(capabilities_type::get(i));
                }, std::forward<decltype(capabilities)>(capabilities));
            }
            else
            {
                return std::forward<decltype(capabilities)>(capabilities);
            }
        }

        template<typename Cache, hmpc::expression E>
        static constexpr decltype(auto) collect_capabilities(auto&& capabilities, Cache const& cache, E expr) noexcept
        {
            if constexpr (Cache::contains(hmpc::detail::tag_of<E>))
            {
                return std::forward<decltype(capabilities)>(capabilities);
            }
            else if constexpr (expr.arity > 0)
            {
                return hmpc::iter::scan_range<expr.arity>([&](auto i, auto&& capabilities) -> decltype(auto)
                {
                    return collect_capabilities(std::forward<decltype(capabilities)>(capabilities), cache, expr.get(i));
                }, add_capabilities(std::forward<decltype(capabilities)>(capabilities), expr));
            }
            else
            {
                return add_capabilities(std::forward<decltype(capabilities)>(capabilities), expr);
            }
        }

        template<typename Cache, hmpc::expression E>
        static constexpr auto state(Cache const& cache, auto& tensors, E expr, auto& handler) HMPC_NOEXCEPT
        {
            if constexpr (Cache::contains(hmpc::detail::tag_of<E>))
            {
                return hmpc::comp::device_accessor(std::get<Cache::index_of(hmpc::detail::tag_of<E>)>(tensors), handler, hmpc::access::read);
            }
            else if constexpr (expr.arity > 0)
            {
                return hmpc::iter::for_packed_range<expr.arity>([&](auto... i)
                {
                    return hmpc::expr::make_state(state(cache, tensors, expr.get(i), handler)...);
                });
            }
            else
            {
                static_assert(expr.arity == 0);
                return expr.state(handler);
            }
        }

        template<typename Cache, hmpc::cacheable_expression E>
        constexpr auto execute_single(Cache const& cache, auto& tensors, E expr) HMPC_NOEXCEPT
        {
            auto get_state = [&](auto& handler)
            {
                if constexpr (expr.arity > 0)
                {
                    return hmpc::iter::for_packed_range<expr.arity>([&](auto... i)
                    {
                        return hmpc::expr::make_state(state(cache, tensors, expr.get(i), handler)...);
                    });
                }
                else
                {
                    static_assert(expr.arity == 0);
                    return expr.state(handler);
                }
            };
            auto get_capability_data = [&](auto const& shape)
            {
                auto capabilities = [&]()
                {
                    if constexpr (expr.arity > 0)
                    {
                        return hmpc::iter::scan_range<expr.arity>([&](auto i, auto&& capabilities) -> decltype(auto)
                        {
                            return collect_capabilities(std::forward<decltype(capabilities)>(capabilities), cache, expr.get(i));
                        }, add_capabilities(hmpc::detail::type_set{}, expr));
                    }
                    else
                    {
                        return add_capabilities(hmpc::detail::type_set{}, expr);
                    }
                }();

                return hmpc::iter::scan_range<capabilities.size>([&](auto i, auto&& data) -> decltype(auto)
                {
                    return add_capability_data(std::forward<decltype(data)>(data), capabilities.get(i), shape);
                }, hmpc::detail::type_map{});
            };
            auto make_capabilities = []<typename Data>(Data const& data, auto const& index, auto const& shape)
            {
                return hmpc::iter::for_packed_range<Data::size>([&](auto... i)
                {
                    return capabilities_type<typename decltype(data.key(i))::type...>(data, index, shape);
                });
            };
            auto& tensor = std::get<Cache::index_of(hmpc::detail::tag_of<E>)>(tensors);

            if constexpr (hmpc::complex_expression<E>)
            {
                auto get_extra_tensor = [&]<typename F>(auto const& key, F const& f) -> decltype(auto)
                {
                    using T = std::invoke_result_t<F>;
                    static_assert(std::derived_from<T, tensor_lookup_value_base>);

                    if (auto lookup = extra_tensors.find(key); lookup != extra_tensors.end())
                    {
                        auto extra = dynamic_cast<T*>(lookup->second.get());
                        HMPC_HOST_ASSERT(extra != nullptr);
                        return *extra;
                    }
                    else
                    {
                        auto [iterator, inserted] = extra_tensors.emplace(key, std::make_unique<T>(f()));
                        HMPC_HOST_ASSERT(inserted);
                        return static_cast<T&>(*iterator->second);
                    }
                };

                return expr(sycl_queue, get_state, get_capability_data, make_capabilities, tensor, get_extra_tensor);
            }
            else
            {
                return sycl_queue.submit([&](auto& handler)
                {
                    auto state = get_state(handler);
                    auto write = hmpc::comp::device_accessor(tensor, handler, hmpc::access::discard_write);
                    auto shape = hmpc::expr::element_shape(expr);
                    auto capability_data = get_capability_data(shape);

                    handler.parallel_for(sycl::range{shape.size()}, [=](hmpc::size i)
                    {
                        auto index = [&]()
                        {
                            if constexpr (hmpc::expr::same_element_shape<E>)
                            {
                                return i;
                            }
                            else
                            {
                                return hmpc::from_linear_index(i, shape);
                            }
                        }();

                        auto capabilities = make_capabilities(capability_data, index, shape);

                        write[index] = E::operator()(state, index, capabilities);
                    });
                });
            }
        }

        template<bool UnpackSingle, typename... Exprs>
        auto operator()(hmpc::bool_constant<UnpackSingle>, Exprs... exprs) HMPC_NOEXCEPT
        {
            auto cache = detail::call_packed([&](auto... args)
            {
                static_assert((not decltype(args)::shape_type::has_placeholder and ...));
                return hmpc::expr::generate_execution_cache(args...);
            }, exprs...);

            using cache_type = decltype(cache);

            static_assert(cache_type::size >= sizeof...(Exprs));

            auto tensors = hmpc::iter::for_packed_range<cache.size>([&](auto... i)
            {
                return std::make_tuple([&]()
                {
                    using value_type = typename std::remove_cvref_t<decltype(cache.get(i))>::value_type;
                    auto shape = cache.get(i).shape();
                    return hmpc::comp::make_tensor<value_type>(shape);
                }()...);
            });

            static constexpr auto expression_index_of = []<hmpc::expression E>(hmpc::detail::type_tag<E> e)
            {
                return cache_type::index_of(
                    e.transform([](auto e) { return hmpc::expr::cache(e); })
                );
            };
            static constexpr auto expression_tuple_indices_of = []<hmpc::expression_tuple E>(hmpc::detail::type_tag<E> e)
            {
                return hmpc::iter::for_packed_range<E::arity>([&](auto... i)
                {
                    return hmpc::core::mdsize{
                        expression_index_of(
                            e.transform([&](auto e){ return e.get(i); })
                        )...
                    };
                });
            };

            constexpr auto expression_count = sizeof...(Exprs);
            constexpr auto result_tensor_indices = std::make_tuple(
                []()
                {
                    if constexpr (hmpc::expression_tuple<Exprs>)
                    {
                        return expression_tuple_indices_of(hmpc::detail::tag_of<Exprs>);
                    }
                    else
                    {
                        return expression_index_of(hmpc::detail::tag_of<Exprs>);
                    }
                }()...
            );

            // TODO: How do we want to check this for mixed (tuple and non-tuple) inputs?
            // hmpc::iter::for_range<expression_count>([&](auto i)
            // {
            //     hmpc::iter::for_range<i + 1, expression_count>([&](auto j)
            //     {
            //         static_assert(std::get<i>(result_tensor_indices) != std::get<j>(result_tensor_indices));
            //     });
            // });

            hmpc::iter::for_range<cache.size>([&](auto i)
            {
                execute_single(cache, tensors, cache.get(i));
            });

            auto to_result = [&]<typename E>(hmpc::detail::type_tag<E>, auto index)
            {
                if constexpr (hmpc::expression_tuple<E>)
                {
                    static_assert(decltype(index)::rank == E::arity);
                    return hmpc::iter::for_packed_range<E::arity>([&](auto... i)
                    {
                        return E::owned_from_parts(std::move(std::get<index.get(i)>(tensors))...);
                    });
                }
                else
                {
                    static_assert(hmpc::expression<E>);
                    return std::move(std::get<index>(tensors));
                }
            };

            if constexpr (UnpackSingle and expression_count == 1)
            {
                using E = std::tuple_element_t<0, std::tuple<Exprs...>>; // using E = Exprs...[0] fails to compile
                return to_result(hmpc::detail::tag_of<E>, std::get<0>(result_tensor_indices));
            }
            else
            {
                return hmpc::iter::for_packed_range<expression_count>([&](auto... i)
                {
                    return std::make_tuple(
                        to_result(hmpc::detail::tag_of<Exprs>, std::get<i>(result_tensor_indices))...
                    );
                });
            };
        }

        template<typename... Exprs>
        auto operator()(Exprs... exprs) HMPC_NOEXCEPT
        {
            return this->operator()(hmpc::constants::yes, exprs...);
        }

        template<typename... Exprs>
        auto operator()(hmpc::as_tuple_tag, Exprs... exprs) HMPC_NOEXCEPT
        {
            return this->operator()(hmpc::constants::no, exprs...);
        }

        void wait() HMPC_NOEXCEPT
        {
            sycl_queue.wait();
        }

        /// Return information about the used device
        device_info info() const
        {
            auto from_range = []<int Dim>(sycl::range<Dim> range)
            {
                if constexpr (Dim == 1)
                {
                    return range.get(0);
                }
                else if constexpr (Dim == 2)
                {
                    return std::array<std::size_t, 2>{range.get(0), range.get(1)};
                }
                else
                {
                    static_assert(Dim == 3);
                    return std::array<std::size_t, 3>{range.get(0), range.get(1), range.get(2)};
                }
            };

            auto from_device_type = [](sycl::info::device_type type)
            {
                switch (type)
                {
                case sycl::info::device_type::cpu:
                    return hmpc::comp::device_type::cpu;
                case sycl::info::device_type::gpu:
                    return hmpc::comp::device_type::gpu;
                default:
                    return hmpc::comp::device_type::unknown;
                }
            };

            decltype(auto) device = sycl_queue.get_info<sycl::info::queue::device>();

            return device_info
            {
                .type = from_device_type(device.get_info<sycl::info::device::device_type>()),
                .name = device.get_info<sycl::info::device::name>(),
                .vendor = device.get_info<sycl::info::device::vendor>(),
                .driver = device.get_info<sycl::info::device::driver_version>(),
                .available = device.get_info<sycl::info::device::is_available>(),
                .address_bits = device.get_info<sycl::info::device::address_bits>(),
                .address_align_bits = device.get_info<sycl::info::device::mem_base_addr_align>(),
                .limits = device_limits
                {
                    .compute_units = device.get_info<sycl::info::device::max_compute_units>(),
                    .sub_devices = device.get_info<sycl::info::device::partition_max_sub_devices>(),
                    .work_item_dimensions = device.get_info<sycl::info::device::max_work_item_dimensions>(),
                    .work_item_size_1d = from_range(device.get_info<sycl::info::device::max_work_item_sizes<1>>()),
                    .work_item_size_2d = from_range(device.get_info<sycl::info::device::max_work_item_sizes<2>>()),
                    .work_item_size_3d = from_range(device.get_info<sycl::info::device::max_work_item_sizes<3>>()),
                    .work_group_size = device.get_info<sycl::info::device::max_work_group_size>(),
                    .sub_groups = device.get_info<sycl::info::device::max_num_sub_groups>(),
                    .parameter_size = device.get_info<sycl::info::device::max_parameter_size>(),
                    .global_memory_size = device.get_info<sycl::info::device::global_mem_size>(),
                    .global_memory_cache_size = device.get_info<sycl::info::device::global_mem_cache_size>(),
                    .global_memory_cache_line_size = device.get_info<sycl::info::device::global_mem_cache_line_size>(),
                    .local_memory_size = device.get_info<sycl::info::device::local_mem_size>(),
                }
            };
        }
    };
}
