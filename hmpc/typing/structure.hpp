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
        typename std::remove_cvref_t<T>::is_collective_structure_element;
    };

    template<typename T, hmpc::size... Dimensions>
    auto default_like(hmpc::comp::tensor<T, Dimensions...> const& tensor)
    {
        return hmpc::comp::make_tensor<T>(tensor.shape());
    }

    template<typename T>
    T default_like(T const&)
    {
        T value;
        return value;
    }

    template<hmpc::structure T>
    T default_like(T const& structure)
    {
        return hmpc::iter::for_packed_range<T::size>([&](auto... i)
        {
            return T::from_parts(default_like(structure.get(i))...);
        });
    }

    template<typename T, hmpc::size... Dimensions>
    auto empty_default(hmpc::shape<Dimensions...> shape)
    {
        return hmpc::comp::make_tensor<T>(shape);
    }

    template<typename T>
    T empty_default(hmpc::shapeless_tag)
    {
        T value;
        return value;
    }

    template<hmpc::structure T, typename State>
    T empty_default(State state)
    {
        return hmpc::iter::for_packed_range<T::size>([&](auto... i)
        {
            return T::from_parts(empty_default((static_cast<void>(i), state))...);
        });
    }
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
            using type = decltype(std::declval<T&&>().get(hmpc::size_constant_of<I>));
        };

        template<typename T, hmpc::size I>
        using structure_element_t = structure_element<T, I>::type;

        template<typename... T>
        struct structure_fields : public hmpc::size_constant<(structure_fields<T>::value + ...)>
        {
        };
        
        template<typename T>
        struct structure_fields<T> : public hmpc::size_constant<1>
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

        template<typename... T>
        constexpr hmpc::size structure_fields_v = structure_fields<T...>::value;

        namespace detail
        {
            template<hmpc::size I, hmpc::size ElementIndex, typename... T>
            struct structure_field_helper;
    
            template<hmpc::size I, hmpc::size ElementIndex, typename T, typename... Rest>
            requires (structure_fields_v<T> <= I)
            struct structure_field_helper<I, ElementIndex, T, Rest...>
            {
                using type = structure_field_helper<I - structure_fields_v<T>, 0, Rest...>::type;
            };
            
            template<hmpc::size I, hmpc::size ElementIndex, typename T, typename... Rest>
            requires (structure_fields_v<T> > I and hmpc::structure<T> and hmpc::structure<structure_element_t<T, ElementIndex>> and structure_fields_v<structure_element_t<T, ElementIndex>> <= I)
            struct structure_field_helper<I, ElementIndex, T, Rest...>
            {
                using type = structure_field_helper<I - structure_fields_v<structure_element_t<T, ElementIndex>>, ElementIndex + 1, T>::type;
            };
    
            template<hmpc::size I, hmpc::size ElementIndex, typename T, typename... Rest>
            requires (structure_fields_v<T> > I and hmpc::structure<T> and hmpc::structure<structure_element_t<T, ElementIndex>> and structure_fields_v<structure_element_t<T, ElementIndex>> > I)
            struct structure_field_helper<I, ElementIndex, T, Rest...>
            {
                using type = structure_field_helper<I, 0, structure_element_t<T, ElementIndex>>::type;
            };
    
            template<hmpc::size I, hmpc::size ElementIndex, typename T, typename... Rest>
            requires (structure_fields_v<T> > I and hmpc::structure<T> and not hmpc::structure<structure_element_t<T, ElementIndex>> and I > 0)
            struct structure_field_helper<I, ElementIndex, T, Rest...>
            {
                using type = structure_field_helper<I - 1, ElementIndex + 1, T>::type;
            };
    
            template<hmpc::size I, hmpc::size ElementIndex, typename T, typename... Rest>
            requires (structure_fields_v<T> > I and hmpc::structure<T> and not hmpc::structure<structure_element_t<T, ElementIndex>> and I == 0)
            struct structure_field_helper<I, ElementIndex, T, Rest...>
            {
                using type = structure_element<T, ElementIndex>::type;
            };

            template<hmpc::size I, hmpc::size ElementIndex, typename T, typename... Rest>
            requires (structure_fields_v<T> > I and not hmpc::structure<T>)
            struct structure_field_helper<I, ElementIndex, T, Rest...>
            {
                static_assert(I == 0);
                using type = T;
            };
        }

        template<hmpc::size I, typename... T>
        struct structure_field
        {
            using type = detail::structure_field_helper<I, 0, T...>::type;
        };

        template<hmpc::size I, typename... T>
        using structure_field_t = structure_field<I, T...>::type;
    }
}
