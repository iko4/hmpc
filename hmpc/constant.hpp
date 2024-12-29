#pragma once

#include <hmpc/config.hpp>

namespace hmpc
{
    namespace traits
    {
        template<typename T>
        struct is_constant : std::false_type
        {
        };

        template<typename T, T V>
        struct is_constant<constant<T, V>> : std::true_type
        {
        };

        template<typename T>
        struct is_constant<T const> : is_constant<T>
        {
        };

        template<typename T>
        constexpr bool is_constant_v = is_constant<T>::value;

        template<typename T>
        struct remove_constant
        {
            using type = T;
        };

        template<typename T, T V>
        struct remove_constant<constant<T, V>>
        {
            using type = T;
        };

        template<typename T>
        struct remove_constant<T const> : remove_constant<T>
        {
        };

        template<typename T>
        using remove_constant_t = remove_constant<T>::type;

        template<typename T, auto Default>
        struct value_or
        {
            static constexpr auto value = Default;
        };

        template<typename T, T V, auto Default>
        struct value_or<constant<T, V>, Default>
        {
            static constexpr auto value = V;
        };

        template<typename T, auto Default>
        constexpr auto value_or_v = value_or<T, Default>::value;
    }

    template<typename T>
    concept is_constant = traits::is_constant_v<T>;

    template<typename T, typename... Ts>
    concept same_without_constant = (std::same_as<traits::remove_constant_t<T>, traits::remove_constant_t<Ts>> and ...);

    template<typename T, typename U>
    concept maybe_constant_of = std::same_as<traits::remove_constant_t<T>, U>;

    template<typename T, typename U>
    concept is_constant_of = is_constant<T> and maybe_constant_of<T, U>;

    template<auto V>
    constexpr constant<decltype(V), V> constant_of = {};

    template<hmpc::size N>
    using size_constant = constant<hmpc::size, N>;
    template<hmpc::size N>
    constexpr size_constant<N> size_constant_of = {};

    template<hmpc::signed_size N>
    using signed_size_constant = constant<hmpc::signed_size, N>;
    template<hmpc::signed_size N>
    constexpr signed_size_constant<N> signed_size_constant_of = {};

    template<typename T>
    concept maybe_signed_size_constant = is_constant_of<T, hmpc::size> or is_constant_of<T, hmpc::signed_size>;

    template<hmpc::rotate N>
    using rotate_constant = constant<hmpc::rotate, N>;
    template<hmpc::rotate N>
    constexpr rotate_constant<N> rotate_constant_of = {};

    template<typename T>
    using zero_constant = constant<T, T{}>;
    template<typename T>
    constexpr zero_constant<T> zero_constant_of = {};

    template<bool Value>
    using bool_constant = constant<bool, Value>;
    template<bool Value>
    constexpr bool_constant<Value> bool_constant_of = {};
    using false_type = bool_constant<false>;
    using true_type = bool_constant<true>;

    template<typename To, typename From, From Value>
    constexpr auto constant_cast(hmpc::constant<From, Value>) noexcept
    {
        return hmpc::constant<To, static_cast<To>(Value)>{};
    }

    template<typename T, T V>
    constexpr T dynamic_value(hmpc::constant<T, V>) noexcept
    {
        return V;
    }

    template<typename T>
        requires (not is_constant<T>)
    constexpr T dynamic_value(T value) noexcept
    {
        return value;
    }
}
