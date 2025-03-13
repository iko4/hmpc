#pragma once

#include <hmpc/comp/accessor.hpp>
#include <hmpc/net/communicator.hpp>
#include <hmpc/net/config.hpp>
#include <hmpc/net/structure.hpp>
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

    public:
        static constexpr party_constant<Id> id = {};

        explicit queue()
            : handle(hmpc::ffi::hmpc_ffi_net_queue_init(id, nullptr))
        {
        }

        explicit queue(hmpc::net::config&& config)
            : handle(hmpc::ffi::hmpc_ffi_net_queue_init(hmpc::core::to_underlying(id), std::move(config).release()))
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
                .sender = hmpc::core::to_underlying(Sender),
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
                .sender = hmpc::core::to_underlying(Sender),
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
                        .sender = hmpc::core::to_underlying(Senders),
                        .size = byte_size,
                    };
                }()...
            };

            auto accessors = std::make_tuple(
                make_accessor(id, id, tensors)...
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

            constexpr auto count = hmpc::size_constant_of<sizeof...(Senders)>;

            auto result = std::make_tuple(
                make_one_result<Ts>(shapes)...
            );

            auto metadata = hmpc::iter::unpack(hmpc::range(count), [&](auto... i)
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
                            .sender = hmpc::core::to_underlying(senders.get(i)),
                            .size = byte_size,
                        };
                    }()...
                };
            });

            auto accessors = make_accessors(id, senders, result);

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
                .receiver = hmpc::core::to_underlying(Receiver),
                .size = byte_size,
            };

            auto result = make_result_storage_with(id, communicator, std::move(tensor));

            // TODO: technically, we do not need to access our own tensor and could think of a way to avoid this
            auto accessors = make_accessors(id, communicator, result);

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
            auto result = make_empty_result_storage<T>(communicator, shape);

            using tensor_type = std::tuple_element_t<0, decltype(result)>;
            using limb_type = tensor_type::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(std::get<0>(result));

            auto metadata = hmpc::ffi::GatherMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .receiver = hmpc::core::to_underlying(Receiver),
                .size = byte_size,
            };

            auto accessors = make_accessors(id, communicator, result);

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
                .receiver = hmpc::core::to_underlying(Receiver),
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

            constexpr auto count = hmpc::size_constant_of<sizeof...(Ts)>;

            auto result = make_empty_multi_result_storage<Ts...>(communicator, shapes...);

            auto extended_receivers = hmpc::iter::scan(hmpc::range(count), [&](auto i, auto parties)
            {
                constexpr auto fields = hmpc::typing::traits::structure_fields<std::tuple_element_t<0, std::tuple_element_t<i, decltype(result)>>>{};
                return hmpc::iter::scan(hmpc::range(fields), [&](auto, auto parties)
                {
                    return parties.append(receivers.get(i));
                }, parties);
            }, hmpc::net::communicator_for<>);

            auto metadata = hmpc::iter::unpack(hmpc::range(count), [&](auto... i)
            {
                return hmpc::iter::collect_enumerated([&]<typename Tensor>(auto j, Tensor&& tensor)
                {
                    using limb_type = std::remove_cvref_t<Tensor>::limb_type;

                    hmpc::net::message_size byte_size = get_byte_size(tensor);

                    return hmpc::ffi::GatherMetadata
                    {
                        .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                        .receiver = hmpc::core::to_underlying(extended_receivers.get(j)),
                        .size = byte_size,
                    };
                }, get<0>(get<i>(result))...);
            });

            auto accessors = make_multi_accessors(id, communicator, result);

            auto data = make_multi_data_ptrs(communicator, accessors);

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

            using tensor_types = std::tuple<Tensors...>;
            constexpr auto count = hmpc::size_constant_of<sizeof...(Tensors)>;
            auto extended_receivers = hmpc::iter::scan(hmpc::range(count), [&](auto i, auto parties)
            {
                constexpr auto fields = hmpc::typing::traits::structure_fields<std::tuple_element_t<i, tensor_types>>{};
                return hmpc::iter::scan(hmpc::range(fields), [&](auto, auto parties)
                {
                    return parties.append(receivers.get(i));
                }, parties);
            }, hmpc::net::communicator_for<>);

            auto metadata = hmpc::iter::collect_enumerated([&]<typename T>(auto i, T& tensor)
            {
                using limb_type = std::remove_cvref_t<T>::limb_type;

                hmpc::net::message_size byte_size = get_byte_size(tensor);

                return hmpc::ffi::GatherMetadata
                {
                    .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                    .receiver = hmpc::core::to_underlying(extended_receivers.get(i)),
                    .size = byte_size,
                };
            }, tensors...);

            auto accessors = hmpc::iter::collect([](auto& tensor)
            {
                return make_accessor(id, id, tensor);
            }, tensors...);

            auto data = make_data_ptrs(extended_receivers, accessors);

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

            auto metadata = hmpc::iter::collect([&]<typename T>(T&& tensor)
            {
                using limb_type = std::remove_cvref_t<T>::limb_type;

                hmpc::net::message_size byte_size = get_byte_size(tensor);

                return hmpc::ffi::AllGatherMetadata
                {
                    .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                    .size = byte_size,
                };
            }, tensor);

            auto result = make_result_storage_with(id, communicator, std::move(tensor));

            auto accessors = make_accessors(id, communicator, result);

            if constexpr (is_multi_accessor(accessors))
            {
                auto data = make_multi_data_ptrs(communicator, accessors);

                if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_multi_all_gather(handle.get(), detail::to_ffi(metadata), detail::to_ffi(communicator), detail::to_2d_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
                {
                    hmpc::ffi::throw_exception(errc);
                }
            }
            else
            {
                static_assert(std::tuple_size_v<decltype(metadata)> == 1);

                auto data = make_data_ptrs(communicator, accessors);

                if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_all_gather(handle.get(), std::get<0>(metadata), detail::to_ffi(communicator), detail::to_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
                {
                    hmpc::ffi::throw_exception(errc);
                }
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
            auto metadata = hmpc::iter::collect([&]<typename Tensor>(Tensor&& tensor)
            {
                using limb_type = std::remove_cvref_t<Tensor>::limb_type;

                hmpc::net::message_size byte_size = get_byte_size(tensor);

                return hmpc::ffi::AllGatherMetadata
                {
                    .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                    .size = byte_size,
                };
            }, tensors...);

            auto result = make_multi_result_storage_with(id, communicator, std::move(tensors)...);

            auto accessors = make_multi_accessors(id, communicator, result);

            auto data = make_multi_data_ptrs(communicator, accessors);

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
            auto metadata = hmpc::iter::collect([&]<typename T>(T&& tensor)
            {
                using limb_type = std::remove_cvref_t<T>::limb_type;

                hmpc::net::message_size byte_size = get_byte_size(tensor);

                return hmpc::ffi::AllGatherMetadata
                {
                    .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                    .size = byte_size,
                };
            }, tensor);

            auto result = make_result_storage_with(id, senders, std::move(tensor));

            auto accessors = make_accessors(id, senders, result);

            if constexpr (is_multi_accessor(accessors))
            {
                auto data = make_multi_data_ptrs(senders, accessors);

                if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_extended_multi_all_gather(handle.get(), detail::to_ffi(metadata), detail::to_ffi(senders), detail::to_ffi(receivers), detail::to_2d_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
                {
                    hmpc::ffi::throw_exception(errc);
                }
            }
            else
            {
                static_assert(std::tuple_size_v<decltype(metadata)> == 1);

                auto data = make_data_ptrs(senders, accessors);

                if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_extended_all_gather(handle.get(), std::get<0>(metadata), detail::to_ffi(senders), detail::to_ffi(receivers), detail::to_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
                {
                    hmpc::ffi::throw_exception(errc);
                }
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
            auto result = make_empty_result_storage<T>(senders, shape);

            using tensor_type = std::tuple_element_t<0, decltype(result)>;
            using limb_type = tensor_type::limb_type;

            hmpc::net::message_size byte_size = get_byte_size(std::get<0>(result));

            auto metadata = hmpc::ffi::AllGatherMetadata
            {
                .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                .size = byte_size,
            };

            auto accessors = make_accessors(id, senders, result);

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
            auto metadata = hmpc::iter::collect([&]<typename T>(T& tensor)
            {
                using limb_type = T::limb_type;

                hmpc::net::message_size byte_size = get_byte_size(tensor);

                return hmpc::ffi::AllGatherMetadata
                {
                    .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                    .size = byte_size,
                };
            }, tensors...);

            auto result = make_multi_result_storage_with(id, senders, std::move(tensors)...);

            auto accessors = make_multi_accessors(id, senders, result);

            auto data = make_multi_data_ptrs(senders, accessors);

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
            constexpr auto count = hmpc::size_constant_of<sizeof...(Ts)>;

            auto result = make_empty_multi_result_storage<Ts...>(senders, shapes...);

            auto metadata = hmpc::iter::unpack(hmpc::range(count), [&](auto... i)
            {
                return hmpc::iter::collect([&]<typename Tensor>(Tensor&& tensor)
                {
                    using limb_type = std::remove_cvref_t<Tensor>::limb_type;

                    hmpc::net::message_size byte_size = get_byte_size(tensor);

                    return hmpc::ffi::AllGatherMetadata
                    {
                        .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                        .size = byte_size,
                    };
                }, get<0>(get<i>(result))...);
            });

            auto accessors = make_multi_accessors(id, senders, result);

            auto data = make_multi_data_ptrs(senders, accessors);

            if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_extended_multi_all_gather(handle.get(), detail::to_ffi(metadata), detail::to_ffi(senders), detail::to_ffi(receivers), detail::to_2d_ffi(data)); errc != hmpc::ffi::SendReceiveErrc::ok)
            {
                hmpc::ffi::throw_exception(errc);
            }

            return result;
        }

        /// All-to-all (as sender and receiver)
        ///
        /// Send `tensor[i]` to `communicator[i]` and receive one tensor for each sent tensor.
        template<party_id... Parties, hmpc::typing::universal_reference_to_rvalue Tensor>
            requires ((Parties == Id) or ...)
        auto all_to_all(communicator<Parties...> communicator, Tensor&& tensor)
        {
            static_assert(std::tuple_size_v<std::remove_cvref_t<Tensor>> == communicator.size);

            auto result = make_result_storage_with(
                id,
                communicator,
                auto(std::get<communicator.index_of(id)>(tensor)) // copy to not use a moved-from object later
            );

            auto send = std::forward<Tensor>(tensor);

            auto send_accessors = make_accessors(id, hmpc::net::communicator{(static_cast<void>(Parties), id)...}, send);
            auto receive_accessors = make_accessors(id, communicator, result);

            if constexpr (is_multi_accessor(send_accessors))
            {
                auto send_data = make_multi_data_ptrs(communicator, send_accessors);
                auto receive_data = make_multi_data_ptrs(communicator, receive_accessors);

                auto metadata = hmpc::iter::collect([&]<typename T>(T&& tensor)
                {
                    using limb_type = std::remove_cvref_t<T>::limb_type;

                    hmpc::net::message_size byte_size = get_byte_size(tensor);

                    return hmpc::ffi::AllToAllMetadata
                    {
                        .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                        .size = byte_size,
                    };
                }, std::get<0>(send));

                if (auto errc = hmpc::ffi::hmpc_ffi_net_queue_multi_all_to_all(handle.get(), detail::to_ffi(metadata), detail::to_ffi(communicator), detail::to_2d_ffi(send_data), detail::to_2d_ffi(receive_data)); errc != hmpc::ffi::SendReceiveErrc::ok)
                {
                    hmpc::ffi::throw_exception(errc);
                }
            }
            else
            {
                static_assert(not is_multi_accessor(send_accessors));
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

            auto result = make_multi_result_storage_with(
                id,
                communicator,
                auto(std::get<communicator.index_of(id)>(tensors))... // copy to not use a moved-from object later
            );

            auto metadata = hmpc::iter::collect([&]<typename Tensor>(Tensor&& tensor)
            {
                using limb_type = std::remove_cvref_t<Tensor>::limb_type;

                hmpc::net::message_size byte_size = get_byte_size(tensor);

                return hmpc::ffi::AllToAllMetadata
                {
                    .datatype = hmpc::net::traits::message_datatype_of_v<limb_type>,
                    .size = byte_size,
                };
            }, std::get<0>(tensors)...);

            auto send = std::make_tuple(std::forward<Tensors>(tensors)...);

            auto send_accessors = make_multi_accessors(id, hmpc::net::communicator{(static_cast<void>(Parties), id)...}, send);
            auto receive_accessors = make_multi_accessors(id, communicator, result);

            auto send_data = make_multi_data_ptrs(communicator, send_accessors);
            auto receive_data = make_multi_data_ptrs(communicator, receive_accessors);

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
