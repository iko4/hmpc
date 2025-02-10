#pragma once

#include <hmpc/config.hpp>
#include <hmpc/constant.hpp>

namespace hmpc
{
    namespace traits
    {
        template<typename T>
        struct element_type
        {
            using type = T;
        };
        template<typename T>
            requires requires { typename T::element_type; T::vector_size; }
        struct element_type<T>
        {
            using type = T::element_type;
        };
        template<typename T>
        using element_type_t = element_type<T>::type;

        template<typename T>
        struct vector_size : hmpc::size_constant<0>
        {
        };
        template<typename T>
            requires requires { typename T::element_type; T::vector_size; }
        struct vector_size<T> : hmpc::size_constant<T::vector_size>
        {
        };
        template<typename T>
        constexpr hmpc::size vector_size_v = vector_size<T>::value;

        template<typename T>
        struct has_limbs : std::false_type
        {
        };
        template<typename T>
            requires requires { typename T::limb_type; T::limb_size; }
        struct has_limbs<T> : std::true_type
        {
        };
        template<typename T>
        constexpr bool has_limbs_v = has_limbs<T>::value;

        template<typename T>
        struct limb_type
        {
            using type = T;
        };
        template<typename T>
            requires (has_limbs_v<T>)
        struct limb_type<T>
        {
            using type = T::limb_type;
        };
        template<typename T>
        using limb_type_t = limb_type<T>::type;

        template<typename T>
        struct limb_size : hmpc::size_constant<1>
        {
        };
        template<typename T>
            requires (has_limbs_v<T>)
        struct limb_size<T> : hmpc::size_constant<T::limb_size>
        {
        };
        template<typename T>
        constexpr hmpc::size limb_size_v = limb_size<T>::value;
    }

    template<typename T>
    concept value = std::same_as<traits::limb_type_t<T>, traits::limb_type_t<traits::element_type_t<T>>> and (traits::limb_size_v<T> == traits::limb_size_v<traits::element_type_t<T>>);

    template<typename T>
    concept scalar = value<T> and (traits::vector_size_v<T> == 0) and std::same_as<T, traits::element_type_t<T>>;

    template<typename T>
    concept vector = value<T> and (traits::vector_size_v<T> > 0) and scalar<traits::element_type_t<T>>;
}
