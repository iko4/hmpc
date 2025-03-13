#pragma once

#include <hmpc/comp/accessor.hpp>
#include <hmpc/core/limb_span.hpp>
#include <hmpc/iter/at_structure.hpp>
#include <hmpc/iter/collect_structure.hpp>
#include <hmpc/net/communicator.hpp>
#include <hmpc/typing/reference.hpp>
#include <hmpc/typing/structure.hpp>

namespace hmpc::net
{
    template<typename T, party_id... Parties, typename State>
    auto make_empty_result_storage(communicator<Parties...>, State state)
    {
        if constexpr (hmpc::collective_structure<T>)
        {
            return T::empty_from(state);
        }
        else
        {
            return std::array
            {
                empty_default<T>((static_cast<void>(Parties), state))...
            };
        }
    }

    template<typename... Ts, party_id... Parties, typename... States>
    auto make_empty_multi_result_storage(communicator<Parties...> communicator, States... states)
    {
        return std::make_tuple(
            make_empty_result_storage<Ts>(communicator, states)...
        );
    }

    template<hmpc::typing::universal_reference_to_rvalue T, party_id Id, party_id... Parties>
    auto make_result_storage_with(hmpc::party_constant<Id> id, communicator<Parties...> communicator, T&& value)
    {
        static_assert(communicator.contains(id), "Party has to be part of the communicator");

        if constexpr (hmpc::collective_structure_element<T>)
        {
            return T::default_owner_with(std::move(value));
        }
        else
        {
            return std::array<std::remove_cvref_t<T>, communicator.size>
            {
                [&]()
                {
                    if constexpr (Parties == Id)
                    {
                        // TODO: think of a way to allowing moving value (note: default_like should not use a moved-from value)
                        return value;
                    }
                    else
                    {
                        return default_like(value);
                    }
                }()...
            };
        }
    }

    template<hmpc::typing::universal_reference_to_rvalue... Ts, party_id Id, party_id... Parties>
    auto make_multi_result_storage_with(hmpc::party_constant<Id> id, communicator<Parties...> communicator, Ts&&... values)
    {
        return std::make_tuple(
            make_result_storage_with(id, communicator, std::forward<Ts>(values))...
        );
    }


    template<typename T, hmpc::size... Dimensions, party_id Id, party_id I>
    auto make_accessor(hmpc::party_constant<Id> id, hmpc::party_constant<I> i, hmpc::comp::tensor<T, Dimensions...>& tensor)
    {
        return hmpc::comp::host_accessor
        {
            tensor,
            [&]()
            {
                if constexpr (id == i)
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

    template<typename T, party_id Id, party_id I>
    auto make_accessor(hmpc::party_constant<Id> id, hmpc::party_constant<I> i, T& value)
    {
        if constexpr (id == i)
        {
            return value.span(hmpc::access::read);
        }
        else
        {
            return value.span(hmpc::access::write);
        }
    }

    template<typename T, party_id Id, party_id... Parties>
    auto make_accessors(hmpc::party_constant<Id> id, communicator<Parties...> communicator, T& values)
    {
        static_assert(std::tuple_size_v<T> == communicator.size);
        using first_element = std::tuple_element_t<0, T>;
        if constexpr (hmpc::structure<first_element>) // return a multi-accessor
        {
            auto make_accessors_for_parties = [&](auto j)
            {
                return hmpc::iter::unpack(hmpc::range(communicator.size), [&](auto... i)
                {
                    return std::make_tuple(
                        make_accessor(
                            id,
                            communicator.get(i),
                            hmpc::iter::at(get<i>(values), j)
                        )...
                    );
                });
            };

            constexpr auto fields = hmpc::typing::traits::structure_fields<first_element>{};
            return hmpc::iter::unpack(hmpc::range(fields), [&](auto... j)
            {
                return std::make_tuple(
                    make_accessors_for_parties(j)...
                );
            });
        }
        else
        {
            return hmpc::iter::unpack(hmpc::range(communicator.size), [&](auto... i)
            {
                return std::make_tuple(
                    make_accessor(
                        id,
                        communicator.get(i),
                        std::get<i>(values)
                    )...
                );
            });
        }
    }

    template<typename T, party_id Id, party_id... Parties>
    auto make_multi_accessors(hmpc::party_constant<Id> id, communicator<Parties...> communicator, T& values)
    {
        constexpr auto size = std::tuple_size<T>{};
        if constexpr (hmpc::iter::unpack(hmpc::range(size), [](auto... i) { return (hmpc::structure<std::tuple_element_t<0, std::tuple_element_t<i, T>>> or ...); })) // some part is a structure
        {
            constexpr auto field_elements = hmpc::iter::unpack(hmpc::range(size), [](auto... i)
            {
                return (hmpc::typing::traits::structure_fields_v<std::tuple_element_t<0, std::tuple_element_t<i, T>>> + ...);
            });

            auto make_accessors_for_parties = [&](this const auto& self, auto j, auto tuple_index)
            {
                using current_first = std::tuple_element_t<0, std::tuple_element_t<tuple_index, T>>;
                if constexpr (hmpc::structure<current_first>)
                {
                    constexpr auto fields = hmpc::typing::traits::structure_fields<current_first>{};
                    if constexpr (fields <= j)
                    {
                        return self(hmpc::iter::prev(j, fields), hmpc::iter::next(tuple_index));
                    }
                    else
                    {
                        return hmpc::iter::unpack(hmpc::range(communicator.size), [&](auto... i)
                        {
                            return std::make_tuple(
                                make_accessor(
                                    id,
                                    communicator.get(i),
                                    hmpc::iter::at(get<i>(std::get<tuple_index>(values)), j)
                                )...
                            );
                        });
                    }
                }
                else
                {
                    if constexpr (j == 0)
                    {
                        return make_accessors(id, communicator, std::get<tuple_index>(values));
                    }
                    else
                    {
                        static_assert(j > 0);
                        return self(hmpc::iter::prev(j), hmpc::iter::next(tuple_index));
                    }
                }
            };

            return hmpc::iter::unpack(hmpc::range(field_elements), [&](auto... j)
            {
                return std::make_tuple(
                    make_accessors_for_parties(j, hmpc::constants::zero)...
                );
            });
        }
        else
        {
            return hmpc::iter::unpack(hmpc::range(size), [&](auto... i)
            {
                return std::make_tuple(
                    make_accessors(id, communicator, std::get<i>(values))...
                );
            });
        }
    }

    template<typename... Ts>
    constexpr auto is_multi_accessor(std::tuple<Ts...>&) noexcept
    {
        if constexpr ((hmpc::typing::traits::is_tuple_v<Ts> and ...))
        {
            return hmpc::constants::yes;
        }
        else
        {
            return hmpc::constants::no;
        }
    }


    template<typename T, typename Access, typename Shape>
    hmpc::net::data_ptr make_data_ptr(hmpc::comp::host_accessor<T, Access, Shape>& accessor)
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

    template<hmpc::limb_span Span>
    hmpc::net::data_ptr make_data_ptr(Span& span)
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
    auto make_data_ptrs(communicator<Parties...> communicator, Accessors& accessors)
    {
        static_assert(std::tuple_size_v<Accessors> == communicator.size);
        return hmpc::iter::unpack(hmpc::range(communicator.size), [&](auto... i)
        {
            return std::array<hmpc::net::data_ptr, communicator.size>
            {
                make_data_ptr(std::get<i>(accessors))...
            };
        });
    }

    template<typename Accessors, party_id... Parties>
    auto make_multi_data_ptrs(communicator<Parties...> communicator, Accessors& accessors)
    {
        constexpr auto size = std::tuple_size<Accessors>{};
        return hmpc::iter::unpack(hmpc::range(size), [&](auto... i)
        {
            return std::array<std::array<hmpc::net::data_ptr, communicator.size>, size>
            {
                make_data_ptrs(communicator, std::get<i>(accessors))...
            };
        });
    }

    template<typename T, hmpc::size... Dimensions>
    auto get_byte_size(hmpc::comp::tensor<T, Dimensions...>& tensor)
    {
        return tensor.get().byte_size();
    }

    template<typename T>
    constexpr auto get_byte_size(T&)
    {
        constexpr auto bit_size = T::limb_size * T::limb_bit_size;
        static_assert(std::numeric_limits<unsigned char>::digits == 8);
        static_assert(bit_size % 8 == 0);
        return bit_size / 8;
    }
}
