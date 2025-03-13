#pragma once

#include <hmpc/net/communicator.hpp>
#include <hmpc/typing/structure.hpp>

#include <tuple>

namespace hmpc::comp::mpc
{
    template<typename T, hmpc::party_id Id, hmpc::party_id... Parties>
    struct share;

    template<typename T, hmpc::party_id... Parties>
    struct shares
    {
        using is_structure = void;
        using is_collective_structure = void;
        static constexpr auto size = hmpc::size_constant_of<sizeof...(Parties)>;
        static constexpr auto communicator = hmpc::net::communicator_for<Parties...>;

        T values[size];

        template<hmpc::size I, typename Self>
        constexpr auto get(this Self&& self, hmpc::size_constant<I> i = {})
        {
            static_assert(I >= 0);
            static_assert(I < size);

            using s = share<T, communicator.get(i), Parties...>;

            return s::from_parts(std::forward<Self>(self).values[I]);
        }

        template<typename State>
        static constexpr auto empty_from(State state)
        {
            return from_parts(hmpc::empty_default<T>((static_cast<void>(Parties), state))...);
        }

        template<hmpc::typing::universal_reference_to_rvalue... Parts>
        static constexpr auto from_parts(Parts&&... parts)
        {
            return shares<std::common_type_t<std::remove_cvref_t<Parts>...>, Parties...>{std::move(parts)...};
        }

        static constexpr auto from_parts(share<T, Parties, Parties...>&&... parts)
        {
            return shares{std::move(parts).value...};
        }
    };

    template<hmpc::size I, typename T, hmpc::party_id... Parties>
    constexpr auto get(shares<T, Parties...>& s)
    {
        return s.get(hmpc::size_constant_of<I>);
    }

    template<hmpc::size I, typename T, hmpc::party_id... Parties>
    constexpr auto get(shares<T, Parties...> const& s)
    {
        return s.get(hmpc::size_constant_of<I>);
    }

    template<typename T, hmpc::party_id Id, hmpc::party_id... Parties>
    struct share
    {
        T value;

        using is_structure = void;
        using is_collective_structure_element = void;
        static constexpr auto size = hmpc::size_constant_of<1>;
        static constexpr auto owner_size = hmpc::size_constant_of<sizeof...(Parties)>;
        static constexpr auto id = hmpc::party_constant_of<Id>;
        static constexpr auto communicator = hmpc::net::communicator_for<Parties...>;

        template<typename Self>
        constexpr auto&& get(this Self&& self, hmpc::size_constant<0>)
        {
            return std::forward<Self>(self).value;
        }

        template<typename Part>
        static constexpr auto from_parts(Part&& part)
        {
            return share{std::forward<Part>(part)};
        }

        static constexpr auto default_owner_with(share&& share)
        {
            using owner = shares<T, Parties...>;
            return owner::from_parts([&]()
            {
                if constexpr (Parties == Id)
                {
                    // TODO: think of a way to allowing moving value (note: default_like should not use a moved-from value)
                    return share.value;
                }
                else
                {
                    return hmpc::default_like(share.value);
                }
            }()...);
        }
    };
}

template<typename T, hmpc::party_id... Parties>
struct std::tuple_size<hmpc::comp::mpc::shares<T, Parties...>> : public std::integral_constant<std::size_t, sizeof...(Parties)>
{
};

template<typename T, hmpc::party_id... Parties, std::size_t I>
struct std::tuple_element<I, hmpc::comp::mpc::shares<T, Parties...>>
{
    using type = hmpc::comp::mpc::share<T, hmpc::net::communicator_for<Parties...>.get(hmpc::size_constant_of<I>), Parties...>;
};
