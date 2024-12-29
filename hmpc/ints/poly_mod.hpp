#pragma once

#include <hmpc/ints/mod.hpp>
#include <hmpc/ints/poly.hpp>

namespace hmpc::ints
{
    template<auto Modulus, hmpc::size Degree, polynomial_representation Representation>
    struct poly_mod;

    template<auto Modulus, hmpc::size Degree>
    struct poly_mod<Modulus, Degree, coefficient_representation>
    {
        using element_type = hmpc::ints::mod<Modulus>;

        static constexpr auto modulus = element_type::modulus;
        static constexpr auto representation = coefficient_representation;

        using limb_type = element_type::limb_type;
        using normal_type = element_type::normal_type;
        static constexpr hmpc::size vector_size = Degree;

        using unsigned_type = hmpc::comp::vector<typename element_type::unsigned_type, vector_size>;
        using signed_type = hmpc::comp::vector<typename element_type::signed_type, vector_size>;

        static constexpr hmpc::size bit_size = element_type::bit_size;
        static constexpr hmpc::size limb_size = element_type::limb_size;

        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator==(poly_mod const&, poly_mod const&) HMPC_NOEXCEPT;
        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator!=(poly_mod const&, poly_mod const&) HMPC_NOEXCEPT;

        friend constexpr poly_mod operator*(element_type const&, poly_mod const&) HMPC_NOEXCEPT;

        friend constexpr poly_mod operator*(poly_mod const&, element_type const&) HMPC_NOEXCEPT;

        friend constexpr poly_mod operator+(poly_mod const&, poly_mod const&) HMPC_NOEXCEPT;
        friend constexpr poly_mod operator-(poly_mod const&, poly_mod const&) HMPC_NOEXCEPT;
    };

    template<auto Modulus, hmpc::size Degree>
    struct poly_mod<Modulus, Degree, number_theoretic_transform_representation>
    {
        using element_type = hmpc::ints::mod<Modulus>;

        static constexpr auto modulus = element_type::modulus;
        static constexpr auto representation = number_theoretic_transform_representation;

        using limb_type = element_type::limb_type;
        using normal_type = element_type::normal_type;
        static constexpr hmpc::size vector_size = Degree;

        static constexpr hmpc::size bit_size = element_type::bit_size;
        static constexpr hmpc::size limb_size = element_type::limb_size;

        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator==(element_type const&, poly_mod const&) HMPC_NOEXCEPT;
        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator!=(element_type const&, poly_mod const&) HMPC_NOEXCEPT;

        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator==(poly_mod const&, element_type const&) HMPC_NOEXCEPT;
        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator!=(poly_mod const&, element_type const&) HMPC_NOEXCEPT;

        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator==(poly_mod const&, poly_mod const&) HMPC_NOEXCEPT;
        friend constexpr hmpc::comp::vector<hmpc::bit, vector_size> operator!=(poly_mod const&, poly_mod const&) HMPC_NOEXCEPT;

        friend constexpr poly_mod operator+(element_type const&, poly_mod const&) HMPC_NOEXCEPT;
        friend constexpr poly_mod operator-(element_type const&, poly_mod const&) HMPC_NOEXCEPT;
        friend constexpr poly_mod operator*(element_type const&, poly_mod const&) HMPC_NOEXCEPT;

        friend constexpr poly_mod operator+(poly_mod const&, element_type const&) HMPC_NOEXCEPT;
        friend constexpr poly_mod operator-(poly_mod const&, element_type const&) HMPC_NOEXCEPT;
        friend constexpr poly_mod operator*(poly_mod const&, element_type const&) HMPC_NOEXCEPT;

        friend constexpr poly_mod operator+(poly_mod const&, poly_mod const&) HMPC_NOEXCEPT;
        friend constexpr poly_mod operator-(poly_mod const&, poly_mod const&) HMPC_NOEXCEPT;
        friend constexpr poly_mod operator*(poly_mod const&, poly_mod const&) HMPC_NOEXCEPT;
    };

    namespace traits
    {
        template<auto Modulus, hmpc::size Degree, polynomial_representation Representation>
        struct coefficient_type<hmpc::ints::poly_mod<Modulus, Degree, Representation>>
        {
            using type = hmpc::ints::poly_mod<Modulus, Degree, coefficient_representation>;
        };

        template<auto Modulus, hmpc::size Degree, polynomial_representation Representation>
        struct number_theoretic_transform_type<hmpc::ints::poly_mod<Modulus, Degree, Representation>>
        {
            using type = hmpc::ints::poly_mod<Modulus, Degree, number_theoretic_transform_representation>;
        };
    }
}
