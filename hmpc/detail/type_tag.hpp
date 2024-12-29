#pragma once

#include <type_traits>

namespace hmpc::detail
{
    template<typename T>
    struct type_tag
    {
        using type = T;

        template<typename F, typename... Args>
        static constexpr auto transform(F&&, type_tag<Args>...) noexcept
        {
            using result_type = std::invoke_result_t<F, T, Args...>;
            return type_tag<result_type>{};
        }
    };

    template<typename T>
    constexpr type_tag<T> tag_of = {};

    template<typename First, typename Second>
    struct type_pair
    {
        using first_type = First;
        using second_type = Second;
    };

    template<typename First, typename Second, typename Third>
    struct type_triple
    {
        using first_type = First;
        using second_type = Second;
        using third_type = Third;
    };

    template<typename First, typename Second>
    constexpr type_tag<type_pair<First, Second>> pair_tag_of = {};

    template<typename First, typename Second, typename Third>
    constexpr type_tag<type_triple<First, Second, Third>> triple_tag_of = {};
}
