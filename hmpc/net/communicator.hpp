#pragma once

#include <hmpc/core/to_underlying.hpp>
#include <hmpc/net/ffi.hpp>

#include <algorithm>
#include <array>

namespace hmpc::net
{
    template<party_id... Parties>
    struct communicator : public hmpc::detail::constant_list<party_id, Parties...>
    {
        template<party_id Id>
        static constexpr auto append(hmpc::party_constant<Id> = {}) noexcept
        {
            return communicator<Parties..., Id>{};
        }

        template<party_id... OtherParties>
        static constexpr auto append(communicator<OtherParties...> = {}) noexcept
        {
            return communicator<Parties..., OtherParties...>{};
        }
    };
    template<party_id... Parties>
    communicator(hmpc::party_constant<Parties>...) -> communicator<Parties...>;

    template<auto... Parties>
    constexpr communicator<party_id{Parties}...> communicator_for = {};

    namespace detail
    {
        template<party_id... Parties>
        constexpr auto to_ffi(communicator<Parties...>) noexcept
        {
            static constexpr std::array<hmpc::ffi::PartyID, sizeof...(Parties)> parties = { hmpc::core::to_underlying(Parties)... };
            static_assert(std::ranges::is_sorted(parties), "Communicator has to be sorted");
            static_assert(std::ranges::adjacent_find(parties) == std::ranges::end(parties), "Communicator has to be free of duplicates");
            return hmpc::ffi::Communicator{ .data = parties.data(), .len = parties.size() };
        }
    }
}
