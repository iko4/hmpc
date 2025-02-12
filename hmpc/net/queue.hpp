#pragma once

#include <hmpc/comp/accessor.hpp>
#include <hmpc/net/communicator.hpp>
#include <hmpc/net/config.hpp>
#include <hmpc/typing/reference.hpp>

namespace hmpc::net
{
    template<party_id Id>
    class queue
    {
    private:
        using deleter_type = decltype([](hmpc::ffi::Queue* queue)
        {
            if (queue != nullptr)
            {
                hmpc::ffi::hmpc_ffi_net_queue_free(queue);
            }
        });
        std::unique_ptr<hmpc::ffi::Queue, deleter_type> handle;

        template<typename T>
        static auto make_one_result(auto shape)
        {
            return hmpc::comp::make_tensor<T>(shape);
        }

        template<typename T>
        static auto make_one_result(hmpc::shapeless_tag)
        {
            T result;
            return result;
        }

        template<typename T, party_id... Parties>
        static auto make_result_storage(communicator<Parties...>, auto shape)
        {
            static_assert(not ((Parties == Id) or ...));

            return std::array
            {
                (static_cast<void>(Parties), hmpc::comp::make_tensor<T>(shape))...
            };
        }

        template<typename T, party_id... Parties>
        static auto make_result_storage(communicator<Parties...>, hmpc::shapeless_tag)
        {
            static_assert(not ((Parties == Id) or ...));

            std::array<T, sizeof...(Parties)> result;

            return result;
        }

        template<typename... Ts, party_id... Parties>
        static auto make_new_2d_result_storage(communicator<Parties...> communicator, auto... shapes)
        {
            return std::make_tuple(
                make_result_storage<Ts>(communicator, shapes)...
            );
        }

        template<typename T, hmpc::size... Dimensions>
        static auto get_byte_size(hmpc::comp::tensor<T, Dimensions...>& tensor)
        {
            return tensor.get().byte_size();
        }

        template<typename T>
        constexpr static auto get_byte_size(T&)
        {
            constexpr auto bit_size = T::limb_size * T::limb_bit_size;
            static_assert(std::numeric_limits<unsigned char>::digits == 8);
            static_assert(bit_size % 8 == 0);
            return bit_size / 8;
        }

        template<typename T, hmpc::size... Dimensions, party_id... Parties>
        static auto make_result_storage(communicator<Parties...> communicator, hmpc::comp::tensor<T, Dimensions...>&& tensor)
        {
            static_assert(((Parties == Id) or ...));

            using tensor_type = hmpc::comp::tensor<T, Dimensions...>;

            auto shape = tensor.shape();

            return std::array<tensor_type, communicator.size>
            {
                [&]()
                {
                    if constexpr (Parties == Id)
                    {
                        return std::move(tensor);
                    }
                    else
                    {
                        return hmpc::comp::make_tensor<T>(shape);
                    }
                }()...
            };
        }

        template<hmpc::typing::universal_reference_to_rvalue T, party_id... Parties>
        static auto make_result_storage(communicator<Parties...> communicator, T&& value)
        {
            static_assert(((Parties == Id) or ...));

            return std::array<std::remove_cvref_t<T>, communicator.size>
            {
                [&]()
                {
                    if constexpr (Parties == Id)
                    {
                        return std::move(value);
                    }
                    else
                    {
                        std::remove_cvref_t<T> v;
                        return v;
                    }
                }()...
            };
        }

        template<party_id... Parties, typename... Tensors>
        static auto make_2d_result_storage(communicator<Parties...> communicator, Tensors&&... tensors)
        {
            return std::make_tuple(
                make_result_storage(communicator, std::forward<Tensors>(tensors))...
            );
        }

        template<typename T, hmpc::size... Dimensions, party_id I>
        static auto make_accessor(hmpc::party_constant<I>, hmpc::comp::tensor<T, Dimensions...>& tensor)
        {
            return hmpc::comp::host_accessor
            {
                tensor,
                [&]()
                {
                    if constexpr (I == Id)
                    {
                        return hmpc::access::read;
                    }
                    else
                    {
                        return hmpc::access::discard_write;
                    }
                }()
            };
        }

        template<typename T, party_id I>
        static auto make_accessor(hmpc::party_constant<I>, T& value)
        {
            if constexpr (I == Id)
            {
                return value.span(hmpc::access::read);
            }
            else
            {
                return value.span(hmpc::access::write);
            }
        }

        template<typename Tensors, party_id... Parties>
        static auto make_accessors(communicator<Parties...> communicator, Tensors& tensors)
        {
            static_assert(std::tuple_size_v<Tensors> == communicator.size);
            return hmpc::iter::for_packed_range<communicator.size>([&](auto... i)
            {
                return std::make_tuple(
                    make_accessor(
                        communicator.get(i),
                        std::get<i>(tensors)
                    )...
                );
            });
        }

        template<typename Tensors, party_id... Parties>
        static auto make_2d_accessors(communicator<Parties...> communicator, Tensors& tensors)
        {
            constexpr auto size = std::tuple_size_v<Tensors>;
            return hmpc::iter::for_packed_range<size>([&](auto... i)
            {
                return std::make_tuple(
                    make_accessors(communicator, std::get<i>(tensors))...
                );
            });
        }

        template<typename T, typename Access, typename Shape>
        static hmpc::net::data_ptr make_data_ptr(hmpc::comp::host_accessor<T, Access, Shape>& accessor)
        {
            if constexpr (hmpc::access::is_read_only<Access>)
            {
                using accessor_type = hmpc::comp::host_accessor<T, Access, Shape>;
                return static_cast<hmpc::net::data_ptr>(const_cast<accessor_type::limb_type*>(accessor.data()));
            }
            else
            {
                static_assert(hmpc::access::is_write_only<Access>);
                return static_cast<hmpc::net::data_ptr>(accessor.data());
            }
        }

        template<typename Span>
        static hmpc::net::data_ptr make_data_ptr(Span& span)
        {
            if constexpr (hmpc::read_only_limb_span<Span>)
            {
                return static_cast<hmpc::net::data_ptr>(const_cast<Span::limb_type*>(span.ptr()));
            }
            else
            {
                static_assert(hmpc::write_only_limb_span<Span>);
                return static_cast<hmpc::net::data_ptr>(span.ptr());
            }
        }

        template<typename Accessors, party_id... Parties>
        static auto make_data_ptrs(communicator<Parties...> communicator, Accessors& accessors)
        {
            static_assert(std::tuple_size_v<Accessors> == communicator.size);
            return hmpc::iter::for_packed_range<communicator.size>([&](auto... i)
            {
                return std::array<hmpc::net::data_ptr, communicator.size>
                {
                    make_data_ptr(std::get<i>(accessors))...
                };
            });
        }

        template<typename Accessors, party_id... Parties>
        static auto make_2d_data_ptrs(communicator<Parties...> communicator, Accessors& accessors)
        {
            constexpr auto size = std::tuple_size_v<Accessors>;
            return hmpc::iter::for_packed_range<size>([&](auto... i)
            {
                return std::array<std::array<hmpc::net::data_ptr, communicator.size>, size>
                {
                    make_data_ptrs(communicator, std::get<i>(accessors))...
                };
            });
        }

    public:
        static constexpr party_id id = Id;

        explicit queue()
            : handle(hmpc::ffi::hmpc_ffi_net_queue_init(id, nullptr))
        {
        }

        explicit queue(hmpc::net::config&& config)
            : handle(hmpc::ffi::hmpc_ffi_net_queue_init(id, std::move(config).release()))
        {
        }

        explicit queue(hmpc::party_constant<Id>)
            : queue()
        {
        }

        explicit queue(hmpc::party_constant<Id>, hmpc::net::config&& config)
            : queue(std::move(config))
        {
        }

        /// Broadcast (as sender)
        ///
        /// Sends `tensor` to all parties in `communicator`.
        template<party_id Sender, typename Tensor, party_id... Parties>
            requires (Sender == Id)
        void broadcast(communicator<Parties...> communicator, hmpc::party_constant<Sender>, Tensor&& tensor)
        {
            // TODO: Adapt for non-tensor inputs
            using limb_type = std::remove_cvref_t<Tensor>::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(tensor);

            auto metadata = hmpc::ffi::BroadcastMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .sender = Sender,
                .size = byte_size,
            };

            hmpc::comp::host_accessor accessor(tensor, hmpc::access::read);
            auto data = static_cast<hmpc::net::data_ptr>(const_cast<limb_type*>(accessor.data()));

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_broadcast(handle.get(), metadata, detail::to_ffi(communicator), data); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }
        }

        /// Broadcast (as receiver)
        ///
        /// Receives one tensor with elements of type `T` and shape `shape` from `Sender`.
        template<typename T, party_id Sender, party_id... Parties>
            requires (Sender != Id)
        auto broadcast(communicator<Parties...> communicator, hmpc::party_constant<Sender>, auto const& shape)
        {
            // TODO: Adapt for non-tensor inputs
            static_assert(((Parties == Id) or ...));

            auto tensor = hmpc::comp::make_tensor<T>(shape);

            using tensor_type = decltype(tensor);
            using limb_type = tensor_type::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(tensor);

            auto metadata = hmpc::ffi::BroadcastMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .sender = Sender,
                .size = byte_size,
            };

            hmpc::comp::host_accessor accessor(tensor, hmpc::access::discard_write);
            auto data = static_cast<hmpc::net::data_ptr>(accessor.data());

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_broadcast(handle.get(), metadata, detail::to_ffi(communicator), data); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return tensor;
        }

        /// Multi broadcast (as sender)
        ///
        /// Sends all `tensors` to all parties in `communicator`.
        template<party_id... Parties, party_id... Senders, typename... Tensors>
            requires ((Senders == Id) and ...)
        void broadcast(hmpc::net::communicator<Parties...> communicator, hmpc::net::communicator<Senders...> senders, Tensors&&... tensors)
        {
            static_assert(sizeof...(Senders) == sizeof...(Tensors));

            auto metadata = std::array
            {
                [&]()
                {
                    using limb_type = Tensors::limb_type;

                    hmpc::net::message_size byte_size = get_byte_size(tensors);

                    return hmpc::ffi::BroadcastMetadata
                    {
                        .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                        .sender = Senders,
                        .size = byte_size,
                    };
                }()...
            };

            auto accessors = std::make_tuple(
                make_accessor(hmpc::party_constant_of<Id>, tensors)...
            );

            auto data = make_data_ptrs(senders, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_multi_broadcast(handle.get(), detail::to_ffi(metadata), detail::to_ffi(communicator), detail::to_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }
        }

        /// Multi broadcast (as receiver)
        ///
        /// For each `T` in `Ts`,
        /// for each `Sender` in `Senders`,
        /// for each `shape` in `shapes`:
        /// Receives one tensor with elements of type `T` and shape `shape` from `Sender`.
        template<typename... Ts, party_id... Parties, party_id... Senders>
            requires ((Senders != Id) and ...)
        auto broadcast(hmpc::net::communicator<Parties...> communicator, hmpc::net::communicator<Senders...> senders, auto const&... shapes)
        {
            static_assert(((Parties == Id) or ...));
            static_assert(sizeof...(Senders) == sizeof...(shapes));
            static_assert(sizeof...(Ts) == sizeof...(shapes));

            constexpr hmpc::size count = sizeof...(Senders);

            auto result = std::make_tuple(
                make_one_result<Ts>(shapes)...
            );

            auto metadata = hmpc::iter::for_packed_range<count>([&](auto... i)
            {
                return std::array
                {
                    [&]()
                    {
                        using limb_type = std::tuple_element_t<i, decltype(result)>::limb_type;

                        hmpc::net::message_size byte_size = get_byte_size(std::get<i>(result));

                        return hmpc::ffi::BroadcastMetadata
                        {
                            .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                            .sender = senders.get(i),
                            .size = byte_size,
                        };
                    }()...
                };
            });

            auto accessors = make_accessors(senders, result);

            auto data = make_data_ptrs(senders, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_multi_broadcast(handle.get(), detail::to_ffi(metadata), detail::to_ffi(communicator), detail::to_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Gather (as receiver that is also part of the senders)
        ///
        /// Receive one tensor from each party in `communicator` (with the same element type and shape as `tensor`).
        /// Own tensor `tensor` is moved into the result.
        template<party_id Receiver, hmpc::typing::universal_reference_to_rvalue Tensor, party_id... Parties>
            requires (Receiver == Id and ((Parties == Id) or ...))
        auto gather(communicator<Parties...> communicator, hmpc::party_constant<Receiver>, Tensor&& tensor)
        {
            using limb_type = Tensor::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(tensor);

            auto metadata = hmpc::ffi::GatherMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .receiver = Receiver,
                .size = byte_size,
            };

            auto result = make_result_storage(communicator, std::move(tensor));

            // TODO: technically, we do not need to access our own tensor and could think of a way to avoid this
            auto accessors = make_accessors(communicator, result);

            auto data = make_data_ptrs(communicator, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_gather(handle.get(), metadata, detail::to_ffi(communicator), detail::to_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Gather (as receiver that is not part of the senders)
        ///
        /// Receive one tensor from each party in `communicator` with element type `T` and shape `shape`.
        template<typename T, party_id Receiver, hmpc::size... Dimensions, party_id... Parties>
            requires (Receiver == Id and not ((Parties == Id) or ...))
        auto gather(communicator<Parties...> communicator, hmpc::party_constant<Receiver>, auto const& shape)
        {
            auto result = make_result_storage<T>(communicator, shape);

            using tensor_type = std::tuple_element_t<0, decltype(result)>;
            using limb_type = tensor_type::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(std::get<0>(result));

            auto metadata = hmpc::ffi::GatherMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .receiver = Receiver,
                .size = byte_size,
            };

            auto accessors = make_accessors(communicator, result);

            auto data = make_data_ptrs(communicator, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_gather(handle.get(), metadata, detail::to_ffi(communicator), detail::to_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Gather (as sender)
        ///
        /// Send `tensor` to `Receiver`.
        template<party_id Receiver, typename Tensor, party_id... Parties>
            requires (Receiver != Id)
        void gather(communicator<Parties...> communicator, hmpc::party_constant<Receiver>, Tensor&& tensor)
        {
            // TODO: Adapt for non-tensor inputs
            static_assert(((Parties == Id) or ...));

            using limb_type = std::remove_cvref_t<Tensor>::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(tensor);

            auto metadata = hmpc::ffi::GatherMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .receiver = Receiver,
                .size = byte_size,
            };

            hmpc::comp::host_accessor accessor(tensor, hmpc::access::read);
            auto data = static_cast<hmpc::net::data_ptr>(const_cast<limb_type*>(accessor.data()));

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_gather(handle.get(), metadata, detail::to_ffi(communicator), detail::to_single_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }
        }

        /// Multi gather (as receiver)
        ///
        /// For each `T` in `Ts`,
        /// for each `shape` in `shapes`:
        /// Receive one tensor from each party in `communicator` with element type `T` and shape `shape`.
        template<typename... Ts, party_id... Parties, party_id... Receivers>
            requires ((Receivers == Id) and ...)
        auto gather(hmpc::net::communicator<Parties...> communicator, hmpc::net::communicator<Receivers...> receivers, auto const&... shapes)
        {
            static_assert(((Parties != Id) and ...));
            static_assert(sizeof...(Receivers) == sizeof...(shapes));
            static_assert(sizeof...(Ts) == sizeof...(shapes));

            constexpr hmpc::size count = sizeof...(Ts);

            auto result = make_new_2d_result_storage<Ts...>(communicator, shapes...);

            auto metadata = hmpc::iter::for_packed_range<count>([&](auto... i)
            {
                return std::array
                {
                    [&]()
                    {
                        using limb_type = std::tuple_element_t<0, std::tuple_element_t<i, decltype(result)>>::limb_type;

                        hmpc::net::message_size byte_size = get_byte_size(std::get<0>(std::get<i>(result)));

                        return hmpc::ffi::GatherMetadata
                        {
                            .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                            .receiver = receivers.get(i),
                            .size = byte_size,
                        };
                    }()...
                };
            });

            auto accessors = make_2d_accessors(communicator, result);

            auto data = make_2d_data_ptrs(communicator, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_multi_gather(handle.get(), detail::to_ffi(metadata), detail::to_ffi(communicator), detail::to_2d_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Multi gather (as sender)
        ///
        /// For `tensor` in `Tensors`,
        /// for `Receiver` in `Receivers`:
        /// Send `tensor` to `Receiver`.
        template<party_id... Parties, party_id... Receivers, typename... Tensors>
            requires ((Receivers != Id) and ...)
        void gather(hmpc::net::communicator<Parties...> communicator, hmpc::net::communicator<Receivers...> receivers, Tensors&&... tensors)
        {
            static_assert(((Parties == Id) or ...));
            static_assert(sizeof...(Receivers) == sizeof...(Tensors));

            auto metadata = std::array
            {
                [&]()
                {
                    using limb_type = std::remove_cvref_t<Tensors>::limb_type;

                    hmpc::net::message_size byte_size = get_byte_size(tensors);

                    return hmpc::ffi::GatherMetadata
                    {
                        .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                        .receiver = Receivers,
                        .size = byte_size,
                    };
                }()...
            };

            auto accessors = std::make_tuple(
                make_accessor(hmpc::party_constant_of<Id>, tensors)...
            );

            auto data = make_data_ptrs(receivers, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_multi_gather(handle.get(), detail::to_ffi(metadata), detail::to_ffi(communicator), detail::to_outer_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }
        }

        /// All-gather (as sender and receiver)
        ///
        /// Send `tensor` to each party in `communicator` and receive one tensor from each party.
        /// Own tensor `tensor` is moved into the result.
        template<hmpc::typing::universal_reference_to_rvalue Tensor, party_id... Parties>
        auto all_gather(communicator<Parties...> communicator, Tensor&& tensor)
        {
            static_assert(((Parties == Id) or ...));

            using limb_type = Tensor::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(tensor);

            auto metadata = hmpc::ffi::AllGatherMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .size = byte_size,
            };

            auto result = make_result_storage(communicator, std::move(tensor));

            auto accessors = make_accessors(communicator, result);

            auto data = make_data_ptrs(communicator, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_all_gather(handle.get(), metadata, detail::to_ffi(communicator), detail::to_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Multi all-gather (as sender and receiver)
        ///
        /// Send each `tensor` in `tensors` to each party in `communicator` and receive the same number of tensors from each party in `senders`.
        /// Own tensors `tensors` are moved into the result.
        template<party_id... Parties, hmpc::typing::universal_reference_to_rvalue... Tensors>
            requires ((Parties == Id) or ...)
        auto all_gather(communicator<Parties...> communicator, Tensors&&... tensors)
        {
            auto metadata = std::array
            {
                [&]()
                {
                    using limb_type = Tensors::limb_type;

                    hmpc::net::message_size byte_size = get_byte_size(tensors);

                    return hmpc::ffi::AllGatherMetadata
                    {
                        .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                        .size = byte_size,
                    };
                }()...
            };

            auto result = make_2d_result_storage(communicator, std::move(tensors)...);

            auto accessors = make_2d_accessors(communicator, result);

            auto data = make_2d_data_ptrs(communicator, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_multi_all_gather(handle.get(), detail::to_ffi(metadata), detail::to_ffi(communicator), detail::to_2d_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Extended all-gather (as sender and receiver)
        ///
        /// Send `tensor` to each party in `receivers` and receive one tensor from each party in `senders`.
        /// Own tensor `tensor` is moved into the result.
        template<hmpc::typing::universal_reference_to_rvalue Tensor, party_id... Senders, party_id... Receivers>
            requires ((Senders == Id) or ...)
        auto all_gather(communicator<Senders...> senders, communicator<Receivers...> receivers, Tensor&& tensor)
        {
            using limb_type = Tensor::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(tensor);

            auto metadata = hmpc::ffi::AllGatherMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .size = byte_size,
            };

            auto result = make_result_storage(senders, std::move(tensor));

            auto accessors = make_accessors(senders, result);

            auto data = make_data_ptrs(senders, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_extended_all_gather(handle.get(), metadata, detail::to_ffi(senders), detail::to_ffi(receivers), detail::to_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Extended all-gather (as receiver)
        ///
        /// Receive one tensor from each party in `senders` with element type `T` and shape `shape`.
        template<typename T, party_id... Senders, party_id... Receivers>
            requires (not ((Senders == Id) or ...))
        auto all_gather(communicator<Senders...> senders, communicator<Receivers...> receivers, auto const& shape)
        {
            auto result = make_result_storage<T>(senders, shape);

            using tensor_type = std::tuple_element_t<0, decltype(result)>;
            using limb_type = tensor_type::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(std::get<0>(result));

            auto metadata = hmpc::ffi::AllGatherMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .size = byte_size,
            };

            auto accessors = make_accessors(senders, result);

            auto data = make_data_ptrs(senders, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_extended_all_gather(handle.get(), metadata, detail::to_ffi(senders), detail::to_ffi(receivers), detail::to_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Extended multi all-gather (as sender and receiver)
        ///
        /// Send each `tensor` in `tensors` to each party in `receivers` and receive the same number of tensors from each party in `senders`.
        /// Own tensors `tensors` are moved into the result.
        template<party_id... Senders, party_id... Receivers, hmpc::typing::universal_reference_to_rvalue... Tensors>
            requires ((Senders == Id) or ...)
        auto all_gather(communicator<Senders...> senders, communicator<Receivers...> receivers, Tensors&&... tensors)
        {
            auto metadata = std::array
            {
                [&]()
                {
                    using limb_type = Tensors::limb_type;

                    hmpc::net::message_size byte_size = get_byte_size(tensors);

                    return hmpc::ffi::AllGatherMetadata
                    {
                        .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                        .size = byte_size,
                    };
                }()...
            };

            auto result = make_2d_result_storage(senders, std::move(tensors)...);

            auto accessors = make_2d_accessors(senders, result);

            auto data = make_2d_data_ptrs(senders, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_extended_multi_all_gather(handle.get(), detail::to_ffi(metadata), detail::to_ffi(senders), detail::to_ffi(receivers), detail::to_2d_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Extended multi all-gather (as receiver)
        ///
        /// Receive tensors from each party in `senders` with element types `Ts` and shapes `shapes`.
        template<typename... Ts, party_id... Senders, party_id... Receivers>
            requires (not ((Senders == Id) or ...))
        auto all_gather(communicator<Senders...> senders, communicator<Receivers...> receivers, auto const&... shapes)
        {
            constexpr hmpc::size count = sizeof...(Ts);

            auto result = make_new_2d_result_storage<Ts...>(senders, shapes...);

            auto metadata = hmpc::iter::for_packed_range<count>([&](auto... i)
            {
                return std::array
                {
                    [&]()
                    {
                        using limb_type = std::tuple_element_t<0, std::tuple_element_t<i, decltype(result)>>::limb_type;

                        hmpc::net::message_size byte_size = get_byte_size(std::get<0>(std::get<i>(result)));

                        return hmpc::ffi::AllGatherMetadata
                        {
                            .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                            .size = byte_size,
                        };
                    }()...
                };
            });

            auto accessors = make_2d_accessors(senders, result);

            auto data = make_2d_data_ptrs(senders, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_extended_multi_all_gather(handle.get(), detail::to_ffi(metadata), detail::to_ffi(senders), detail::to_ffi(receivers), detail::to_2d_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// Multi all-to-all (as sender and receiver)
        ///
        /// For `tensors` in `tensors`:
        /// Send `tensors[i]` to `communicator[i]` and receive one tensor for each sent tensor.
        template<party_id... Parties, hmpc::typing::universal_reference_to_rvalue... Tensors>
            requires ((Parties == Id) or ...)
        auto all_to_all(communicator<Parties...> communicator, Tensors&&... tensors)
        {
            static_assert(((std::tuple_size_v<std::remove_cvref_t<Tensors>> == communicator.size) and ...));

            constexpr hmpc::size count = sizeof...(Tensors);

            auto result = make_2d_result_storage(
                communicator,
                auto(std::get<communicator.index_of(hmpc::party_constant_of<Id>)>(std::forward<Tensors>(tensors)))... // copy to not use a moved-from object later
            );

            auto metadata = hmpc::iter::for_packed_range<count>([&](auto... i)
            {
                return std::array
                {
                    [&]()
                    {
                        using limb_type = std::tuple_element_t<0, std::tuple_element_t<i, decltype(result)>>::limb_type;

                        hmpc::net::message_size byte_size = get_byte_size(std::get<0>(std::get<i>(result)));

                        return hmpc::ffi::AllToAllMetadata
                        {
                            .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                            .size = byte_size,
                        };
                    }()...
                };
            });

            auto send = std::make_tuple(std::forward<Tensors>(tensors)...);

            auto send_accessors = make_2d_accessors(communicator, send);
            auto receive_accessors = make_2d_accessors(communicator, result);

            auto send_data = make_2d_data_ptrs(communicator, send_accessors);
            auto receive_data = make_2d_data_ptrs(communicator, receive_accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_multi_all_to_all(handle.get(), detail::to_ffi(metadata), detail::to_ffi(communicator), detail::to_2d_ffi(send_data), detail::to_2d_ffi(receive_data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        void wait()
        {
            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_wait(handle.get()); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }
        }

        hmpc::net::statistics stats()
        {
            return hmpc::ffi::hmpc_ffi_net_queue_network_statistics(handle.get());
        }
    };
}
