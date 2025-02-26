#pragma once

#include <hmpc/comp/tensor.hpp>
#include <hmpc/constant.hpp>
#include <hmpc/iter/for_packed_range.hpp>

#include <type_traits>

namespace hmpc
{
    template<typename T>
    concept structure = requires
    {
        std::remove_cvref_t<T>::size;
        typename std::remove_cvref_t<T>::is_structure;
    };

    template<typename T>
    concept collective_structure = structure<T> and requires
    {
        typename std::remove_cvref_t<T>::is_collective_structure;
    };

    template<typename T>
    concept collective_structure_element = requires
    {
        std::remove_cvref_t<T>::rank;
        typename std::remove_cvref_t<T>::is_collective_structure_element;
    };
}

namespace hmpc::typing
{
    namespace traits
    {
        template<typename T, hmpc::size I>
        struct structure_element;

        template<hmpc::structure T, hmpc::size I>
        requires (I < std::remove_cvref_t<T>::size)
        struct structure_element<T, I>
        {
            using type = decltype(std::declval<T>().get(hmpc::size_constant_of<I>));
        };

        template<typename T, hmpc::size I>
        using structure_element_t = structure_element<T, I>::type;

        template<typename T>
        struct structure_fields : public hmpc::size_constant<1>
        {
        };

        template<hmpc::structure T>
        struct structure_fields<T> : public hmpc::size_constant<
            hmpc::iter::for_packed_range<std::remove_cvref_t<T>::size>([](auto... i)
            {
                return (structure_fields<structure_element_t<T, i>>::value + ...);
            })
        >
        {
        };

        template<typename T>
        constexpr hmpc::size structure_fields_v = structure_fields<T>::value;

        template<typename T, hmpc::size I, hmpc::size ElementIndex = 0>
        struct structure_field;

        template<hmpc::structure T, hmpc::size I, hmpc::size ElementIndex>
        requires (hmpc::structure<structure_element_t<T, ElementIndex>> and structure_fields_v<structure_element_t<T, ElementIndex>> <= I)
        struct structure_field<T, I, ElementIndex> : public structure_field<T, I - structure_fields_v<structure_element_t<T, ElementIndex>>, ElementIndex + 1>
        {
        };

        template<hmpc::structure T, hmpc::size I, hmpc::size ElementIndex>
        requires (hmpc::structure<structure_element_t<T, ElementIndex>> and structure_fields_v<structure_element_t<T, ElementIndex>> > I)
        struct structure_field<T, I, ElementIndex> : public structure_field<structure_element_t<T, ElementIndex>, I>
        {
        };

        template<hmpc::structure T, hmpc::size I, hmpc::size ElementIndex>
        requires (not hmpc::structure<structure_element_t<T, ElementIndex>> and I > 0)
        struct structure_field<T, I, ElementIndex> : public structure_field<T, I - 1, ElementIndex + 1>
        {
        };

        template<hmpc::structure T, hmpc::size I, hmpc::size ElementIndex>
        requires (not hmpc::structure<structure_element_t<T, ElementIndex>> and I == 0)
        struct structure_field<T, I, ElementIndex> : public structure_element<T, ElementIndex>
        {
        };

        template<typename T, hmpc::size I>
        using structure_field_t = structure_field<T, I>::type;
    }
}
