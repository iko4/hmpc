#pragma once

#include <hmpc/comp/mpc/share.hpp>
#include <hmpc/detail/unique_tag.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/typing/reference.hpp>

namespace hmpc::expr::mpc
{
    template<typename T, hmpc::net::party_id Id, hmpc::net::party_id... Parties>
    struct share_expression;

    template<typename Communicator, typename... Shares>
    struct shares_expression;

    // cannot have multiple parameter packs in class definition, so we wrap the parties in a communicator
    template<typename... Shares, hmpc::net::party_id... Parties>
    struct shares_expression<hmpc::net::communicator<Parties...>, Shares...>
    {
        using is_tuple = void;
        static_assert(sizeof...(Shares) == sizeof...(Parties));
        static constexpr hmpc::size arity = sizeof...(Shares);
        static constexpr auto communicator = hmpc::net::communicator_for<Parties...>;

        std::tuple<Shares...> shares;

        template<hmpc::size I>
        constexpr auto get(hmpc::size_constant<I> i) const
        {
            return share_expression<std::tuple_element_t<I, decltype(shares)>, communicator.get(i), Parties...>{std::get<i>(shares)};
        }

        template<typename... Others>
        static constexpr auto from_parts(Others... others)
        {
            return shares_expression<hmpc::net::communicator<Parties...>, Others...>{std::make_tuple(others...)};
        }

        constexpr auto reconstruct() const
        {
            return hmpc::iter::for_packed_range<arity>([&](auto... i)
            {
                using namespace hmpc::expr::operators;
                return (std::get<i>(shares) + ...);
            });
        }
    };

    template<typename T, hmpc::net::party_id Id, hmpc::net::party_id... Parties>
    struct share_expression
    {
        using value_type = T;

        value_type value;

        using is_tuple = void;
        static constexpr hmpc::size arity = 1;
        static constexpr auto id = hmpc::party_constant_of<Id>;
        static constexpr auto communicator = hmpc::net::communicator_for<Parties...>;

        constexpr auto get(hmpc::size_constant<0>) const
        {
            return value;
        }

        template<typename Other>
        static constexpr auto from_parts(Other other)
        {
            return share_expression<Other, Id, Parties...>{other};
        }

        template<hmpc::typing::universal_reference_to_rvalue Other>
        static constexpr auto owned_from_parts(Other&& value)
        {
            return hmpc::comp::mpc::share<std::remove_cvref_t<Other>, Id, Parties...>{std::move(value)};
        }

        template<typename Other>
        constexpr auto operator+(share_expression<Other, Id, Parties...> other) const
        {
            using namespace hmpc::expr::operators;
            return share_expression<decltype(value + other.value), Id, Parties...>{value + other.value};
        }

        template<typename Other>
        constexpr auto operator+(Other other) const
        {
            using namespace hmpc::expr::operators;
            if constexpr (communicator.get(hmpc::constants::zero) == id)
            {
                return share_expression<decltype(value + other), Id, Parties...>{value + other};
            }
            else
            {
                return *this;
            }
        }

        template<typename Other>
        constexpr auto operator-(share_expression<Other, Id, Parties...> other) const
        {
            using namespace hmpc::expr::operators;
            return share_expression<decltype(value - other.value), Id, Parties...>{value - other.value};
        }

        template<typename Other>
        constexpr auto operator-(Other other) const
        {
            using namespace hmpc::expr::operators;
            if constexpr (communicator.get(hmpc::constants::zero) == id)
            {
                return share_expression<decltype(value - other), Id, Parties...>{value - other};
            }
            else
            {
                return *this;
            }
        }

        template<typename Left>
        friend constexpr auto operator*(Left left, share_expression right)
        {
            using namespace hmpc::expr::operators;
            return share_expression<decltype(left * right.value), Id, Parties...>{left * right.value};
        }

        template<typename Other, hmpc::net::party_id OtherId, hmpc::net::party_id... OtherParties>
        friend constexpr auto operator*(share_expression<Other, OtherId, OtherParties...> left, share_expression right) = delete;
    };

    template<typename T, hmpc::net::party_id Id, hmpc::net::party_id... Parties>
    constexpr auto share(T value, hmpc::party_constant<Id>, hmpc::net::communicator<Parties...>)
    {
        return share_expression<T, Id, Parties...>{value};
    }

    template<auto Tag = []{}, typename T, hmpc::size... Dimensions, hmpc::net::party_id Id, hmpc::net::party_id... Parties>
    constexpr auto share(hmpc::comp::mpc::share<hmpc::comp::tensor<T, Dimensions...>, Id, Parties...>& s)
    {
        return share(hmpc::expr::tensor<Tag, T, Dimensions...>(s.value), s.id, s.communicator);
    }

    template<typename... Shares, hmpc::net::party_id... Parties>
    constexpr auto shares(share_expression<Shares, Parties, Parties...>... shares)
    {
        return shares_expression<hmpc::net::communicator<Parties...>, Shares...>::from_parts(shares.value...);
    }

    template<auto Tag = []{}, typename T, hmpc::size... Dimensions, hmpc::net::party_id... Parties>
    constexpr auto shares(hmpc::comp::mpc::shares<hmpc::comp::tensor<T, Dimensions...>, Parties...>& shares)
    {
        constexpr hmpc::size size = sizeof...(Parties);
        return hmpc::iter::for_packed_range<size>([&](auto... i)
        {
            return shares_expression<hmpc::net::communicator<Parties...>, decltype(Parties)...>::from_parts(
                hmpc::expr::tensor<hmpc::detail::unique_tag(Tag, i), T, Dimensions...>(shares.values[i])...
            );
        });
    }
}
