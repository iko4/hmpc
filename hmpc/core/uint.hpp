#pragma once

#include <hmpc/assert.hpp>
#include <hmpc/constant.hpp>

#include <bit>
#include <limits>
#include <utility>

#define HMPC_COMPARISON_OPERATOR(T, OP) \
    friend constexpr T<bool> operator OP(T left, T right) HMPC_NOEXCEPT \
    { \
        return T<bool>{left.data OP right.data}; \
    }
#define HMPC_ASSIGNMENT_OPERATOR(T, OP) \
    constexpr T& operator OP##=(T other) HMPC_NOEXCEPT \
    { \
        data OP## = other.data; \
        return *this; \
    }
#define HMPC_SHIFT_ASSIGNMENT_OPERATOR(T, OP) \
    template<hmpc::size Shift> \
    constexpr T& operator OP##=(hmpc::size_constant<Shift> shift) HMPC_NOEXCEPT \
    { \
        static_assert(Shift >= 0); \
        static_assert(Shift < bit_size); \
        data OP## = shift; \
        return *this; \
    } \
    consteval T& operator OP##=(hmpc::size shift) \
    { \
        HMPC_COMPILETIME_ASSERT(shift >= 0); \
        HMPC_COMPILETIME_ASSERT(shift < bit_size); \
        data OP## = shift; \
        return *this; \
    }
#define HMPC_ROTATE_ASSIGNMENT_OPERATOR(T, OP, NAME) \
    template<hmpc::rotate Rotation> \
    constexpr T& operator OP##=(hmpc::rotate_constant<Rotation>) HMPC_NOEXCEPT \
    { \
        static_assert(static_cast<hmpc::size>(Rotation) >= 0); \
        static_assert(static_cast<hmpc::size>(Rotation) < bit_size); \
        data = NAME(data, static_cast<hmpc::size>(Rotation)); \
        return *this; \
    }
#define HMPC_OPERATOR_FROM_ASSIGNMENT_OPERATOR(T, OP) \
    friend constexpr T operator OP(T left, T right) HMPC_NOEXCEPT \
    { \
        return left OP## = right; \
    }
#define HMPC_OPERATOR_FROM_SHIFT_ASSIGNMENT_OPERATOR(T, OP) \
    template<hmpc::size Shift> \
    friend constexpr T operator OP(T value, hmpc::size_constant<Shift> shift) HMPC_NOEXCEPT \
    { \
        return value OP## = shift; \
    } \
    friend consteval T operator OP(T value, hmpc::size shift) \
    { \
        return value OP## = shift; \
    }
#define HMPC_ROTATE_OPERATOR(T, OP, NAME) \
    template<hmpc::rotate Rotation> \
    friend constexpr T operator OP(T value, hmpc::rotate_constant<Rotation>) HMPC_NOEXCEPT \
    { \
        static_assert(static_cast<hmpc::size>(Rotation) >= 0); \
        static_assert(static_cast<hmpc::size>(Rotation) < bit_size); \
        T result; \
        result.data = NAME(value.data, static_cast<hmpc::size>(Rotation)); \
        return result; \
    }
#define HMPC_COMPARISON_OPERATORS(T) \
    HMPC_COMPARISON_OPERATOR(T, ==) \
    HMPC_COMPARISON_OPERATOR(T, !=) \
    HMPC_COMPARISON_OPERATOR(T, >) \
    HMPC_COMPARISON_OPERATOR(T, <) \
    HMPC_COMPARISON_OPERATOR(T, >=) \
    HMPC_COMPARISON_OPERATOR(T, <=)
#define HMPC_ASSIGNMENT_OPERATORS(T) \
    HMPC_ASSIGNMENT_OPERATOR(T, +) \
    HMPC_ASSIGNMENT_OPERATOR(T, -) \
    HMPC_ASSIGNMENT_OPERATOR(T, *) \
    consteval T& operator/=(T other) \
    { \
        HMPC_COMPILETIME_ASSERT(other != T{}); \
        data /= other.data; \
        return *this; \
    } \
    HMPC_ASSIGNMENT_OPERATOR(T, &) \
    HMPC_ASSIGNMENT_OPERATOR(T, |) \
    HMPC_ASSIGNMENT_OPERATOR(T, ^) \
    HMPC_SHIFT_ASSIGNMENT_OPERATOR(T, <<) \
    HMPC_SHIFT_ASSIGNMENT_OPERATOR(T, >>) \
    HMPC_ROTATE_ASSIGNMENT_OPERATOR(T, <<, std::rotl) \
    HMPC_ROTATE_ASSIGNMENT_OPERATOR(T, >>, std::rotr)
#define HMPC_OPERATORS_FROM_ASSIGNMENT_OPERATORS(T) \
    HMPC_OPERATOR_FROM_ASSIGNMENT_OPERATOR(T, +) \
    HMPC_OPERATOR_FROM_ASSIGNMENT_OPERATOR(T, -) \
    HMPC_OPERATOR_FROM_ASSIGNMENT_OPERATOR(T, *) \
    friend consteval T operator/(T left, T right) \
    { \
        return left /= right; \
    } \
    HMPC_OPERATOR_FROM_ASSIGNMENT_OPERATOR(T, &) \
    HMPC_OPERATOR_FROM_ASSIGNMENT_OPERATOR(T, |) \
    HMPC_OPERATOR_FROM_ASSIGNMENT_OPERATOR(T, ^) \
    HMPC_OPERATOR_FROM_SHIFT_ASSIGNMENT_OPERATOR(T, <<) \
    HMPC_OPERATOR_FROM_SHIFT_ASSIGNMENT_OPERATOR(T, >>)
#define HMPC_ROTATE_OPERATORS(T) \
    HMPC_ROTATE_OPERATOR(T, <<, std::rotl) \
    HMPC_ROTATE_OPERATOR(T, >>, std::rotr)
#define HMPC_UNARY_OPERATOR(T, OP) \
    constexpr T operator OP() const HMPC_NOEXCEPT \
    { \
        return T{static_cast<underlying_type>(OP data)}; \
    }
#define HMPC_UNARY_OPERATORS(T) \
    HMPC_UNARY_OPERATOR(T, -) \
    HMPC_UNARY_OPERATOR(T, ~)

namespace hmpc::core
{
    // uint<T> forward declared in config

    template<>
    struct uint<bool>
    {
        using underlying_type = bool;

        static constexpr hmpc::size bit_size = 1;

        /// Data member
        underlying_type data;

        /// Default constructor
        constexpr uint() HMPC_NOEXCEPT = default;
        /// Constructor from underlying
        explicit constexpr uint(underlying_type value) HMPC_NOEXCEPT
            : data{value}
        {
        }
        /// Constructor from other integer
        /// As integer types convert non-zero integers to true when converting to bool, we cannot use standard casts as below.
        /// Instead, we extract the lowest bit.
        template<typename OtherUnderlying>
        explicit constexpr uint(uint<OtherUnderlying> other) HMPC_NOEXCEPT
            : data{static_cast<underlying_type>(other.data bitand OtherUnderlying{1})}
        {
            static_assert(other.bit_size > bit_size);
        }
        // No constructor from smaller integer

        HMPC_COMPARISON_OPERATORS(uint)

        HMPC_OPERATORS_FROM_ASSIGNMENT_OPERATORS(uint)

        template<hmpc::rotate Rotation>
        friend constexpr uint operator<<(uint value, hmpc::rotate_constant<Rotation> rotation) HMPC_NOEXCEPT
        {
            static_assert(rotation == 0);
            return value;
        }

        template<hmpc::rotate Rotation>
        friend constexpr uint operator>>(uint value, hmpc::rotate_constant<Rotation> rotation) HMPC_NOEXCEPT
        {
            static_assert(rotation == 0);
            return value;
        }

        constexpr uint& operator+=(uint other) HMPC_NOEXCEPT
        {
            data ^= other.data;
            return *this;
        }

        constexpr uint& operator-=(uint other) HMPC_NOEXCEPT
        {
            return *this += other;
        }

        constexpr uint& operator*=(uint other) HMPC_NOEXCEPT
        {
            data &= other.data;
            return *this;
        }

        consteval uint& operator/=(uint other)
        {
            HMPC_COMPILETIME_ASSERT(other != uint{});
            return *this;
        }

        constexpr uint& operator&=(uint other) HMPC_NOEXCEPT
        {
            return *this *= other;
        }

        constexpr uint& operator|=(uint other) HMPC_NOEXCEPT
        {
            data |= other.data;
            return *this;
        }

        constexpr uint& operator^=(uint other) HMPC_NOEXCEPT
        {
            return *this += other;
        }

        template<hmpc::size Shift>
        constexpr uint& operator<<=(hmpc::size_constant<Shift>) HMPC_NOEXCEPT
        {
            static_assert(Shift == 0);
            return *this;
        }

        template<hmpc::size Shift>
        constexpr uint& operator>>=(hmpc::size_constant<Shift>) HMPC_NOEXCEPT
        {
            static_assert(Shift == 0);
            return *this;
        }

        template<hmpc::rotate Rotation>
        constexpr uint& operator<<=(hmpc::rotate_constant<Rotation> rotation) HMPC_NOEXCEPT
        {
            static_assert(rotation == 0);
            return *this;
        }

        template<hmpc::rotate Rotation>
        constexpr uint& operator>>=(hmpc::rotate_constant<Rotation> rotation) HMPC_NOEXCEPT
        {
            static_assert(rotation == 0);
            return *this;
        }

        consteval uint& operator<<=(hmpc::size shift)
        {
            HMPC_COMPILETIME_ASSERT(shift == 0);
            return *this;
        }

        consteval uint& operator>>=(hmpc::size shift)
        {
            HMPC_COMPILETIME_ASSERT(shift == 0);
            return *this;
        }

        constexpr uint operator compl() const HMPC_NOEXCEPT
        {
            return not *this;
        }

        constexpr uint operator-() const HMPC_NOEXCEPT
        {
            return *this;
        }

        constexpr uint operator not() const HMPC_NOEXCEPT
        {
            return uint{not data};
        }

        friend constexpr uint operator and(uint left, uint right) HMPC_NOEXCEPT
        {
            return left & right;
        }

        friend constexpr uint operator or(uint left, uint right) HMPC_NOEXCEPT
        {
            return left | right;
        }

        explicit constexpr operator bool() const HMPC_NOEXCEPT
        {
            return data;
        }

        explicit constexpr operator hmpc::size() const HMPC_NOEXCEPT
        {
            return static_cast<hmpc::size>(data);
        }
    };

    constexpr bool bool_cast(uint<bool> bit) noexcept
    {
        return bit.data;
    }

    template<typename Underlying>
    struct uint
    {
        using underlying_type = Underlying;

        static_assert(std::numeric_limits<underlying_type>::radix == 2);
        static_assert(std::numeric_limits<underlying_type>::digits > 0);
        static_assert(std::numeric_limits<underlying_type>::is_integer);
        static_assert(not std::numeric_limits<underlying_type>::is_signed);
        static_assert(std::numeric_limits<underlying_type>::is_exact);
        static_assert(std::numeric_limits<underlying_type>::is_modulo);
        static_assert(std::numeric_limits<underlying_type>::min() == 0);
        static_assert(std::numeric_limits<underlying_type>::min() == underlying_type{});
        static_assert(std::numeric_limits<underlying_type>::min() == -underlying_type{});
        static_assert(std::numeric_limits<underlying_type>::max() == static_cast<underlying_type>(compl underlying_type{}));
        static_assert(std::numeric_limits<underlying_type>::max() == static_cast<underlying_type>(-underlying_type{1}));

        static constexpr hmpc::size bit_size = std::numeric_limits<underlying_type>::digits;

        /// Data member
        underlying_type data;

        /// Default constructor
        constexpr uint() HMPC_NOEXCEPT = default;
        /// Constructor from underlying
        constexpr uint(underlying_type value) HMPC_NOEXCEPT
            : data{value}
        {
        }
        /// Constructor from other integer
        template<typename OtherUnderlying>
        explicit constexpr uint(uint<OtherUnderlying> other) HMPC_NOEXCEPT
            : data{static_cast<underlying_type>(other.data)}
        {
        }

        HMPC_COMPARISON_OPERATORS(uint)

        HMPC_OPERATORS_FROM_ASSIGNMENT_OPERATORS(uint)

        HMPC_ROTATE_OPERATORS(uint)

        HMPC_ASSIGNMENT_OPERATORS(uint)

        HMPC_UNARY_OPERATORS(uint)

        explicit constexpr operator hmpc::size() const HMPC_NOEXCEPT
            requires (std::cmp_greater_equal(std::numeric_limits<hmpc::size>::max(), std::numeric_limits<underlying_type>::max()))
        {
            return static_cast<hmpc::size>(data);
        }
    };

#define HMPC_HAS_UINT1
    using uint1 = uint<bool>;
#ifdef UINT8_MAX
    #define HMPC_HAS_UINT8
    using uint8 = uint<std::uint8_t>;
#endif
#ifdef UINT16_MAX
    #define HMPC_HAS_UINT16
    using uint16 = uint<std::uint16_t>;
#endif
#ifdef UINT32_MAX
    #define HMPC_HAS_UINT32
    using uint32 = uint<std::uint32_t>;
#endif
#ifdef UINT64_MAX
    #define HMPC_HAS_UINT64
    using uint64 = uint<std::uint64_t>;
#endif
}

namespace hmpc
{
    template<bit Bit>
    using bit_constant = constant<bit, Bit>;
    template<bit Bit>
    constexpr bit_constant<Bit> bit_constant_of = {};

    constexpr bool bool_cast(bit value) noexcept
    {
        return hmpc::core::bool_cast(value);
    }
}

#undef HMPC_COMPARISON_OPERATOR
#undef HMPC_ASSIGNMENT_OPERATOR
#undef HMPC_SHIFT_ASSIGNMENT_OPERATOR
#undef HMPC_ROTATE_ASSIGNMENT_OPERATOR
#undef HMPC_OPERATOR_FROM_ASSIGNMENT_OPERATOR
#undef HMPC_OPERATOR_FROM_SHIFT_ASSIGNMENT_OPERATOR
#undef HMPC_ROTATE_OPERATOR
#undef HMPC_COMPARISON_OPERATORS
#undef HMPC_ASSIGNMENT_OPERATORS
#undef HMPC_OPERATORS_FROM_ASSIGNMENT_OPERATORS
#undef HMPC_ROTATE_OPERATORS
#undef HMPC_UNARY_OPERATOR
#undef HMPC_UNARY_OPERATORS

template<typename Underlying, typename Char>
struct HMPC_FMTLIB::formatter<hmpc::core::uint<Underlying>, Char>
{
    using type = hmpc::core::uint<Underlying>;

    static constexpr bool is_specialized = true;

    constexpr auto parse(auto& ctx)
    {
        auto it = ctx.begin();
        for (; (it != ctx.end() and *it != Char{'}'}); ++it)
        {
        }
        return it;
    }

    auto format(type value, auto& ctx) const
    {
        static_assert(type::bit_size >= 4);
        static_assert(type::bit_size % 4 == 0);
        return HMPC_FMTLIB::format_to(ctx.out(), "0x{:0{}x}", value.data, type::bit_size / 4);
    }
};

template<typename Char>
struct HMPC_FMTLIB::formatter<hmpc::core::uint<bool>, Char>
{
    using type = hmpc::core::uint<bool>;

    static constexpr bool is_specialized = true;

    constexpr auto parse(auto& ctx)
    {
        auto it = ctx.begin();
        for (; (it != ctx.end() and *it != Char{'}'}); ++it)
        {
        }
        return it;
    }

    auto format(type value, auto& ctx) const
    {
        return HMPC_FMTLIB::format_to(ctx.out(), "0b{:}", static_cast<unsigned int>(value.data));
    }
};
