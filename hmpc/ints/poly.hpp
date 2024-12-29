#pragma once

#include <hmpc/comp/vector.hpp>

namespace hmpc::ints
{
    enum class polynomial_representation
    {
        coefficient,
        number_theoretic_transform
    };
    constexpr auto coefficient_representation = polynomial_representation::coefficient;
    constexpr auto number_theoretic_transform_representation = polynomial_representation::number_theoretic_transform;

    template<typename Integer, hmpc::size Degree>
    struct poly
    {
        using element_type = Integer;

        static constexpr auto representation = coefficient_representation;

        using limb_type = element_type::limb_type;
        using normal_type = element_type::normal_type;
        static constexpr hmpc::size vector_size = Degree;

        static constexpr hmpc::size bit_size = element_type::bit_size;
        static constexpr hmpc::size limb_size = element_type::limb_size;

        template<typename OtherInteger>
        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator==(poly<Integer, Degree> const&, poly<OtherInteger, Degree> const&) HMPC_NOEXCEPT;
        template<typename OtherInteger>
        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator!=(poly<Integer, Degree> const&, poly<OtherInteger, Degree> const&) HMPC_NOEXCEPT;

        template<typename OtherInteger>
        friend constexpr poly<decltype(std::declval<Integer>() + std::declval<OtherInteger>()), Degree> operator+(poly<Integer, Degree> const&, poly<OtherInteger, Degree> const&) HMPC_NOEXCEPT;
        template<typename OtherInteger>
        friend constexpr poly<decltype(std::declval<Integer>() + std::declval<OtherInteger>()), Degree> operator-(poly<Integer, Degree> const&, poly<OtherInteger, Degree> const&) HMPC_NOEXCEPT;
    };

    namespace traits
    {
        template<typename T>
        struct coefficient_type
        {
        };
        template<typename Integer, hmpc::size Degree>
        struct coefficient_type<hmpc::ints::poly<Integer, Degree>>
        {
            using type = hmpc::ints::poly<Integer, Degree>;
        };
        template<typename T>
        using coefficient_type_t = coefficient_type<T>::type;

        template<typename T>
        struct number_theoretic_transform_type
        {
        };
        template<typename T>
        using number_theoretic_transform_type_t = number_theoretic_transform_type<T>::type;
    }
}
