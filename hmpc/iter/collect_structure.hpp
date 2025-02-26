#pragma once

#include <hmpc/config.hpp>
#include <hmpc/iter/enumerate_structure.hpp>
#include <hmpc/iter/for_packed_range.hpp>
#include <hmpc/typing/structure.hpp>

#include <type_traits>

namespace hmpc::iter
{
    namespace detail
    {
        template<typename F, typename T, hmpc::size I>
        using enumerate_structure_field_result_t = std::invoke_result_t<F, hmpc::typing::traits::structure_field_t<T, I>>;
    }

    template<hmpc::structure T, typename F>
    constexpr auto collect(T const& tuple, F const& f) HMPC_NOEXCEPT
    {
        constexpr hmpc::size size = hmpc::typing::traits::structure_fields_v<T>;
        return hmpc::iter::for_packed_range<size>([&](auto... i)
        {
            using first_type = detail::enumerate_structure_field_result_t<F, T, 0>;
            constexpr bool all_same = (std::same_as<first_type, detail::enumerate_structure_field_result_t<F, T, i>> and ...);
            std::conditional_t<all_same, std::array<first_type, size>, std::tuple<detail::enumerate_structure_field_result_t<F, T, i>...>> result;
            hmpc::iter::enumerate(tuple, [&](auto j, auto const& value)
            {
                std::get<j>(result) = f(value);
            });
            return result;
        });
    }
}
