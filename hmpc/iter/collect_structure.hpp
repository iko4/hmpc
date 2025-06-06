#pragma once

#include <hmpc/config.hpp>
#include <hmpc/iter/at_structure.hpp>
#include <hmpc/iter/for_packed_range.hpp>
#include <hmpc/typing/structure.hpp>

#include <type_traits>

namespace hmpc::iter
{
    namespace detail
    {
        template<typename F, hmpc::size I, typename... T>
        using invoke_at_structure_field_t = std::invoke_result_t<F, hmpc::typing::traits::structure_field_t<I, T...>>;
        template<typename F, hmpc::size I, typename... T>
        using invoke_enumerate_structure_field_t = std::invoke_result_t<F, hmpc::size_constant<I>, hmpc::typing::traits::structure_field_t<I, T...>>;
    }

    template<hmpc::structure T, typename F>
    constexpr auto collect(T&& tuple, F const& f) HMPC_NOEXCEPT
    {
        constexpr hmpc::size size = hmpc::typing::traits::structure_fields_v<T>;
        return hmpc::iter::for_packed_range<size>([&](auto... i)
        {
            using first_type = detail::invoke_at_structure_field_t<F, 0, T&&>;
            constexpr bool all_same = (std::same_as<first_type, detail::invoke_at_structure_field_t<F, i, T>> and ...);
            using result_type = std::conditional_t<all_same, std::array<first_type, size>, std::tuple<detail::invoke_at_structure_field_t<F, i, T>...>>;
            
            return result_type{f(at(tuple, i))...};
        });
    }

    template<typename F, typename... Ts>
    constexpr auto collect(F const& f, Ts&&... values) HMPC_NOEXCEPT
    {
        constexpr hmpc::size size = hmpc::typing::traits::structure_fields_v<Ts...>;
        return hmpc::iter::for_packed_range<size>([&](auto... i)
        {
            using first_type = detail::invoke_at_structure_field_t<F, 0, Ts&&...>;
            constexpr bool all_same = (std::same_as<first_type, detail::invoke_at_structure_field_t<F, i, Ts...>> and ...);
            using result_type = std::conditional_t<all_same, std::array<first_type, size>, std::tuple<detail::invoke_at_structure_field_t<F, i, Ts...>...>>;
            
            return result_type{f(at(i, values...))...};
        });
    }

    template<hmpc::structure T, typename F>
    constexpr auto collect_enumerated(T&& tuple, F const& f) HMPC_NOEXCEPT
    {
        constexpr hmpc::size size = hmpc::typing::traits::structure_fields_v<T>;
        return hmpc::iter::for_packed_range<size>([&](auto... i)
        {
            using first_type = detail::invoke_enumerate_structure_field_t<F, 0, T&&>;
            constexpr bool all_same = (std::same_as<first_type, detail::invoke_enumerate_structure_field_t<F, i, T>> and ...);
            using result_type = std::conditional_t<all_same, std::array<first_type, size>, std::tuple<detail::invoke_enumerate_structure_field_t<F, i, T>...>>;

            return result_type{f(i, at(tuple, i))...};
        });
    }

    template<typename F, typename... Ts>
    constexpr auto collect_enumerated(F const& f, Ts&&... values) HMPC_NOEXCEPT
    {
        constexpr hmpc::size size = hmpc::typing::traits::structure_fields_v<Ts...>;
        return hmpc::iter::for_packed_range<size>([&](auto... i)
        {
            using first_type = detail::invoke_enumerate_structure_field_t<F, 0, Ts...>;
            constexpr bool all_same = (std::same_as<first_type, detail::invoke_enumerate_structure_field_t<F, i, Ts...>> and ...);
            using result_type = std::conditional_t<all_same, std::array<first_type, size>, std::tuple<detail::invoke_enumerate_structure_field_t<F, i, Ts...>...>>;
            
            return result_type{f(i, at(i, values...))...};
        });
    }
}
