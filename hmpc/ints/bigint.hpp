#pragma once

#include <hmpc/detail/utility.hpp>
#include <hmpc/ints/num/add.hpp>
#include <hmpc/ints/num/bit_and.hpp>
#include <hmpc/ints/num/bit_copy.hpp>
#include <hmpc/ints/num/bit_not.hpp>
#include <hmpc/ints/num/bit_or.hpp>
#include <hmpc/ints/num/bit_xor.hpp>
#include <hmpc/ints/num/compare.hpp>
#include <hmpc/ints/num/divide.hpp>
#include <hmpc/ints/num/is_normal.hpp>
#include <hmpc/ints/num/multiply.hpp>
#include <hmpc/ints/num/negate.hpp>
#include <hmpc/ints/num/shift_left.hpp>
#include <hmpc/ints/num/shift_right.hpp>
#include <hmpc/ints/num/subtract.hpp>

#define HMPC_TEMPLATE_COMPARISON_OPERATOR(T, OP, FUNCTION) \
    template<hmpc::size OtherBits, hmpc::signedness OtherSignedness, typename OtherNormalization> \
    friend constexpr hmpc::bit operator OP(T<Bits, Signedness, Limb, Normalization> const& left, T<OtherBits, OtherSignedness, Limb, OtherNormalization> const& right) HMPC_NOEXCEPT \
    { \
        return hmpc::ints::num::FUNCTION(left, right); \
    }
#define HMPC_TEMPLATE_COMPARISON_OPERATORS(T) \
    HMPC_TEMPLATE_COMPARISON_OPERATOR(T, ==, equal_to) \
    HMPC_TEMPLATE_COMPARISON_OPERATOR(T, !=, not_equal_to) \
    HMPC_TEMPLATE_COMPARISON_OPERATOR(T, >, greater) \
    HMPC_TEMPLATE_COMPARISON_OPERATOR(T, <, less) \
    HMPC_TEMPLATE_COMPARISON_OPERATOR(T, >=, greater_equal) \
    HMPC_TEMPLATE_COMPARISON_OPERATOR(T, <=, less_equal)
#define HMPC_TEMPLATE_OPERATOR(T, OP, FUNCTION, RESULT_BITS, RESULT_SIGNEDNESS) \
    template<hmpc::size OtherBits, hmpc::signedness OtherSignedness, typename OtherNormalization> \
    friend constexpr auto operator OP(T<Bits, Signedness, Limb, Normalization> const& left, T<OtherBits, OtherSignedness, Limb, OtherNormalization> const& right) HMPC_NOEXCEPT \
    { \
        T<RESULT_BITS, RESULT_SIGNEDNESS, Limb, hmpc::access::traits::common_normal_t<Normalization, OtherNormalization>> result; \
        hmpc::ints::num::FUNCTION(result, left, right); \
        return result; \
    }
#define HMPC_TEMPLATE_SHIFT_OPERATOR(T, OP, FUNCTION, RESULT_BITS) \
    template<hmpc::size Shift> \
    friend constexpr auto operator OP(T<Bits, Signedness, Limb, Normalization> const& value, hmpc::size_constant<Shift> shift) HMPC_NOEXCEPT \
    { \
        T<RESULT_BITS, Signedness, Limb, Normalization> result; \
        hmpc::ints::num::FUNCTION(result, value, shift); \
        return result; \
    }
#define HMPC_TEMPLATE_OPERATORS(T) \
    HMPC_TEMPLATE_OPERATOR(T, +, add, (Bits == 0 ? OtherBits : (OtherBits == 0 ? Bits : std::max(Bits, OtherBits) + 1)), hmpc::common_signedness(Signedness, OtherSignedness)) \
    HMPC_TEMPLATE_OPERATOR(T, -, subtract, (OtherBits == 0 ? Bits : std::max(Bits, OtherBits) + 1), hmpc::with_sign) \
    HMPC_TEMPLATE_OPERATOR(T, *, multiply, (Bits == 0 ? 0 : (Bits == 1 ? OtherBits : (OtherBits == 0 ? 0 : (OtherBits == 1 ? Bits : Bits + OtherBits)))), hmpc::common_signedness(Signedness, OtherSignedness)) \
    HMPC_TEMPLATE_OPERATOR(T, &, bit_and, (std::max(Bits, OtherBits)), hmpc::common_signedness(Signedness, OtherSignedness)) \
    HMPC_TEMPLATE_OPERATOR(T, |, bit_or, (std::max(Bits, OtherBits)), hmpc::common_signedness(Signedness, OtherSignedness)) \
    HMPC_TEMPLATE_OPERATOR(T, ^, bit_xor, (std::max(Bits, OtherBits)), hmpc::common_signedness(Signedness, OtherSignedness)) \
    HMPC_TEMPLATE_SHIFT_OPERATOR(T, <<, shift_left, (Bits == 0 ? 0 : Bits + Shift)) \
    HMPC_TEMPLATE_SHIFT_OPERATOR(T, >>, shift_right, (Bits > Shift ? Bits - Shift : 0))

namespace hmpc::ints
{
    template<hmpc::size Bits, hmpc::signedness Signedness, typename Limb = hmpc::default_limb, typename Normalization = hmpc::access::normal_tag>
    struct bigint
    {
        using limb_type = Limb;
        using normal_type = Normalization;

        static constexpr hmpc::size bit_size = Bits;
        static constexpr hmpc::signedness signedness = Signedness;

        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);

        /// Data member
        hmpc::core::bit_array<bit_size, limb_type, signedness, normal_type> data;

        template<typename Self, hmpc::access::can_access<Self> Access = hmpc::access::traits::access_like_t<Self>>
        constexpr auto span(this Self&& self, Access access = {}) HMPC_NOEXCEPT
        {
            return self.data.span(access);
        }

        template<typename Self, hmpc::access::can_access<Self> Access = hmpc::access::traits::access_like_t<Self>>
        constexpr auto compiletime_span(this Self&& self, Access access = {}) HMPC_NOEXCEPT
        {
            return self.data.compiletime_span(access);
        }

        constexpr bigint operator compl() const HMPC_NOEXCEPT
        {
            bigint result;
            hmpc::ints::num::bit_not(result, *this);
            return result;
        }

        constexpr auto operator-() const HMPC_NOEXCEPT
        {
            bigint<bit_size + hmpc::is_unsigned(signedness), hmpc::with_sign, limb_type, normal_type> result;
            hmpc::ints::num::negate(result, *this);
            return result;
        }

        template<hmpc::size OtherBits, hmpc::signedness OtherSignedness, typename OtherNormal>
        explicit constexpr operator bigint<OtherBits, OtherSignedness, limb_type, OtherNormal>() const HMPC_NOEXCEPT
        {
            bigint<OtherBits, OtherSignedness, limb_type, OtherNormal> result;
            hmpc::ints::num::bit_copy(result, *this);
            return result;
        }

        HMPC_TEMPLATE_COMPARISON_OPERATORS(bigint)
        HMPC_TEMPLATE_OPERATORS(bigint)

        template<hmpc::size OtherBits>
            requires (hmpc::is_unsigned(signedness))
        friend consteval auto operator/(bigint<Bits, Signedness, Limb, Normalization> const& left, bigint<OtherBits, Signedness, Limb, Normalization> const& right)
        {
            bigint<Bits, signedness, Limb, Normalization> result;
            hmpc::ints::num::divide(result, hmpc::core::compiletime_nullspan<Limb>, left, right);
            return result;
        }

        template<hmpc::size OtherBits>
            requires (hmpc::is_unsigned(signedness))
        friend consteval auto operator%(bigint<Bits, Signedness, Limb, Normalization> const& left, bigint<OtherBits, Signedness, Limb, Normalization> const& right)
        {
            bigint<OtherBits, signedness, Limb, Normalization> result;
            hmpc::ints::num::divide(hmpc::core::compiletime_nullspan<Limb>, result, left, right);
            return result;
        }
    };


    template<hmpc::size Bits, typename Limb = hmpc::default_limb, typename Normalization = hmpc::access::normal_tag>
    using sbigint = bigint<Bits, hmpc::with_sign, Limb, Normalization>;

    template<hmpc::size Bits, typename Limb = hmpc::default_limb, typename Normalization = hmpc::access::normal_tag>
    using ubigint = bigint<Bits, hmpc::without_sign, Limb, Normalization>;

    template<typename Limb = hmpc::default_limb>
    constexpr auto zero = ubigint<0, Limb>{};

    template<typename Limb = hmpc::default_limb>
    constexpr auto one = ubigint<1, Limb>{1};
}

#undef HMPC_TEMPLATE_COMPARISON_OPERATOR
#undef HMPC_TEMPLATE_COMPARISON_OPERATORS
#undef HMPC_TEMPLATE_OPERATOR
#undef HMPC_TEMPLATE_SHIFT_OPERATOR
#undef HMPC_TEMPLATE_OPERATORS

template<hmpc::size Bits, hmpc::signedness Signedness, typename Limb, typename Normalization, typename Char>
struct HMPC_FMTLIB::formatter<hmpc::ints::bigint<Bits, Signedness, Limb, Normalization>, Char>
{
    using type = hmpc::ints::bigint<Bits, Signedness, Limb, Normalization>;
    using span_type = decltype(std::declval<type const&>().span(hmpc::access::read));
    HMPC_FMTLIB::formatter<span_type, Char> underlying_formatter;

    static constexpr bool is_specialized = true;

    constexpr auto parse(auto& ctx)
    {
        return underlying_formatter.parse(ctx);
    }

    auto format(type const& value, auto& ctx) const
    {
        return underlying_formatter.format(value.span(hmpc::access::read), ctx);
    }
};
