#pragma once

#include <hmpc/ints/poly.hpp>
#include <hmpc/ints/poly_mod.hpp>

namespace hmpc::ints
{
    template<hmpc::size Degree>
    struct bit_monomial_element
    {
        // should not instantiate this type like any other
        using limb_type = void;
        using normal_type = hmpc::access::normal_tag;
        static constexpr hmpc::size bit_size = 0;
        static constexpr hmpc::size limb_size = 0;

        std::optional<hmpc::size> degree;

        constexpr bit_monomial_element(std::optional<hmpc::size> degree) noexcept
            : degree(degree)
        {
        }
    };

    template<hmpc::size Degree>
    struct bit_monomial
    {
        using element_type = bit_monomial_element<Degree>;

        static constexpr auto representation = coefficient_representation;

        using limb_type = element_type::limb_type;
        using normal_type = element_type::normal_type;
        static constexpr hmpc::size vector_size = Degree;

        static constexpr hmpc::size bit_size = element_type::bit_size;
        static constexpr hmpc::size limb_size = element_type::limb_size;

        template<auto Modulus>
        friend constexpr hmpc::ints::poly_mod<Modulus, Degree, coefficient_representation> operator*(bit_monomial const&, hmpc::ints::poly_mod<Modulus, Degree, coefficient_representation> const&);
        template<auto Modulus>
        friend constexpr hmpc::ints::poly_mod<Modulus, Degree, coefficient_representation> operator*(hmpc::ints::poly_mod<Modulus, Degree, coefficient_representation> const&, bit_monomial const&);

        template<typename Integer>
        friend constexpr hmpc::ints::poly<decltype(-std::declval<Integer>()), Degree> operator*(bit_monomial const&, hmpc::ints::poly<Integer, Degree> const&);
        template<typename Integer>
        friend constexpr hmpc::ints::poly<decltype(-std::declval<Integer>()), Degree> operator*(hmpc::ints::poly<Integer, Degree> const&, bit_monomial const&);
    };

    namespace traits
    {
        template<hmpc::size Degree>
        struct coefficient_type<hmpc::ints::bit_monomial<Degree>>
        {
            using type = hmpc::ints::bit_monomial<Degree>;
        };
    }
}
