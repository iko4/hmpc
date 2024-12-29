#pragma once

#include <hmpc/value.hpp>

namespace hmpc::comp
{
    template<hmpc::scalar T, hmpc::size N>
    struct vector
    {
        using element_type = T;
        using limb_type = hmpc::traits::limb_type_t<element_type>;

        static constexpr hmpc::size vector_size = N;
        static constexpr hmpc::size limb_size = hmpc::traits::limb_size_v<element_type>;

        template<typename S>
        friend constexpr vector<hmpc::bit, vector_size> operator==(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<hmpc::bit, vector_size> operator!=(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<hmpc::bit, vector_size> operator<(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<hmpc::bit, vector_size> operator>(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<hmpc::bit, vector_size> operator<=(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<hmpc::bit, vector_size> operator>=(vector const&, vector<S, N> const&);

        template<typename S>
        friend constexpr vector<decltype(std::declval<T>() + std::declval<S>()), vector_size> operator+(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<decltype(std::declval<T>() - std::declval<S>()), vector_size> operator-(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<decltype(std::declval<T>() * std::declval<S>()), vector_size> operator*(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<decltype(std::declval<T>() & std::declval<S>()), vector_size> operator&(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<decltype(std::declval<T>() | std::declval<S>()), vector_size> operator|(vector const&, vector<S, N> const&);
        template<typename S>
        friend constexpr vector<decltype(std::declval<T>() ^ std::declval<S>()), vector_size> operator^(vector const&, vector<S, N> const&);

        template<typename S>
        friend constexpr auto abs(vector<S, N> const&) -> vector<decltype(abs(std::declval<S>())), vector_size>;
    };
}
