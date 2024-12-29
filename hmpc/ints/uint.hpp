#pragma once

#include <hmpc/detail/utility.hpp>
#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/num/convert.hpp>
#include <hmpc/ints/num/add.hpp>
#include <hmpc/ints/num/bit_and.hpp>
#include <hmpc/ints/num/bit_not.hpp>
#include <hmpc/ints/num/bit_or.hpp>
#include <hmpc/ints/num/bit_xor.hpp>
#include <hmpc/ints/num/compare.hpp>
#include <hmpc/ints/num/multiply.hpp>
#include <hmpc/ints/num/negate.hpp>
#include <hmpc/ints/num/shift_left.hpp>
#include <hmpc/ints/num/shift_right.hpp>
#include <hmpc/ints/num/subtract.hpp>

#define HMPC_COMPARISON_OPERATOR(T, OP, FUNCTION) \
    friend constexpr hmpc::bit operator OP(T const& left, T const& right) HMPC_NOEXCEPT \
    { \
        return hmpc::ints::num::FUNCTION(left, right); \
    }
#define HMPC_COMPARISON_OPERATORS(T) \
    HMPC_COMPARISON_OPERATOR(T, ==, equal_to) \
    HMPC_COMPARISON_OPERATOR(T, !=, not_equal_to) \
    HMPC_COMPARISON_OPERATOR(T, >, greater) \
    HMPC_COMPARISON_OPERATOR(T, <, less) \
    HMPC_COMPARISON_OPERATOR(T, >=, greater_equal) \
    HMPC_COMPARISON_OPERATOR(T, <=, less_equal)
#define HMPC_OPERATOR(T, OP, FUNCTION) \
    friend constexpr T operator OP(T const& left, T const& right) HMPC_NOEXCEPT \
    { \
        T result; \
        hmpc::ints::num::FUNCTION(result, left, right); \
        return result; \
    }
#define HMPC_SHIFT_OPERATOR(T, OP, FUNCTION) \
    template<hmpc::size Shift> \
    friend constexpr T operator OP(T const& value, hmpc::size_constant<Shift> shift) HMPC_NOEXCEPT \
    { \
        T result; \
        hmpc::ints::num::FUNCTION(result, value, shift); \
        return result; \
    }
#define HMPC_OPERATORS(T) \
    HMPC_OPERATOR(T, +, add) \
    HMPC_OPERATOR(T, -, subtract) \
    HMPC_OPERATOR(T, *, multiply) \
    HMPC_OPERATOR(T, &, bit_and) \
    HMPC_OPERATOR(T, |, bit_or) \
    HMPC_OPERATOR(T, ^, bit_xor) \
    HMPC_SHIFT_OPERATOR(T, <<, shift_left) \
    HMPC_SHIFT_OPERATOR(T, >>, shift_right)
#define HMPC_ASSIGNMENT_OPERATOR(T, OP, FUNCTION) \
    constexpr T& operator OP##=(T const& other) HMPC_NOEXCEPT \
    { \
        hmpc::ints::num::FUNCTION(*this, *this, other); \
        return *this; \
    }
#define HMPC_SHIFT_ASSIGNMENT_OPERATOR(T, OP, FUNCTION) \
    template<hmpc::size Shift> \
    constexpr T& operator OP##=(hmpc::size_constant<Shift> shift) HMPC_NOEXCEPT \
    { \
        hmpc::ints::num::FUNCTION(*this, *this, shift); \
        return *this; \
    }
#define HMPC_ASSIGNMENT_OPERATORS(T) \
    HMPC_ASSIGNMENT_OPERATOR(T, +, add) \
    HMPC_ASSIGNMENT_OPERATOR(T, -, subtract) \
    HMPC_ASSIGNMENT_OPERATOR(T, *, multiply) \
    HMPC_ASSIGNMENT_OPERATOR(T, &, bit_and) \
    HMPC_ASSIGNMENT_OPERATOR(T, |, bit_or) \
    HMPC_ASSIGNMENT_OPERATOR(T, ^, bit_xor) \
    HMPC_SHIFT_ASSIGNMENT_OPERATOR(T, <<, shift_left) \
    HMPC_SHIFT_ASSIGNMENT_OPERATOR(T, >>, shift_right)
#define HMPC_UNARY_OPERATOR(T, OP, FUNCTION) \
    constexpr T operator OP() const HMPC_NOEXCEPT \
    { \
        T result; \
        hmpc::ints::num::FUNCTION(result, *this); \
        return result; \
    }
#define HMPC_UNARY_OPERATORS(T) \
    HMPC_UNARY_OPERATOR(T, -, negate) \
    HMPC_UNARY_OPERATOR(T, ~, bit_not)

namespace hmpc::ints
{
    template<hmpc::size Bits, typename Limb = hmpc::default_limb, typename Normalization = hmpc::access::unnormal_tag>
    struct uint;

    template<typename Limb, typename Normalization>
    struct uint<0, Limb, Normalization>
    {
        using limb_type = Limb;
        using normal_type = Normalization;

        static constexpr hmpc::size bit_size = 0;

        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = 0;
    };

    template<hmpc::size Bits, typename Limb, typename Normalization>
    struct uint
    {
        using limb_type = Limb;
        using normal_type = Normalization;

        static constexpr hmpc::size bit_size = Bits;

        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);

        /// Data member
        limb_type data[limb_size];

        // No constructors

        template<typename OtherLimb>
        friend constexpr void convert(uint& result, uint<bit_size, OtherLimb> const& value) HMPC_NOEXCEPT
        {
            hmpc::core::num::convert(result.span(hmpc::access::write), value.span());
        }

        template<typename Self, hmpc::access::can_access<Self> Access = hmpc::access::traits::access_like_t<Self>>
        constexpr auto span(this Self&& self, Access = {}) HMPC_NOEXCEPT
        {
            return hmpc::core::unsigned_bit_span<bit_size, limb_type, Access, normal_type>{self.data};
        }

        HMPC_COMPARISON_OPERATORS(uint)

        HMPC_OPERATORS(uint)

        HMPC_ASSIGNMENT_OPERATORS(uint)

        HMPC_UNARY_OPERATORS(uint)
    };
}

#undef HMPC_COMPARISON_OPERATOR
#undef HMPC_COMPARISON_OPERATORS
#undef HMPC_OPERATOR
#undef HMPC_SHIFT_OPERATOR
#undef HMPC_OPERATORS
#undef HMPC_ASSIGNMENT_OPERATOR
#undef HMPC_SHIFT_ASSIGNMENT_OPERATOR
#undef HMPC_ASSIGNMENT_OPERATORS
#undef HMPC_UNARY_OPERATOR
#undef HMPC_UNARY_OPERATORS

template<hmpc::size Bits, typename Limb, typename Normalization, typename Char>
struct HMPC_FMTLIB::formatter<hmpc::ints::uint<Bits, Limb, Normalization>, Char>
{
    using type = hmpc::ints::uint<Bits, Limb, Normalization>;
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
