#pragma once

#include <hmpc/core/constant_bit_span.hpp>
#include <hmpc/ints/bigint.hpp>
#include <hmpc/ints/num/modulo.hpp>
#include <hmpc/ints/num/select.hpp>
#include <hmpc/ints/numeric.hpp>

#include <format>

#define HMPC_COMPARISON_OPERATOR(T, OP, FUNCTION) \
    friend constexpr hmpc::bit operator OP(T const& left, T const& right) HMPC_NOEXCEPT \
    { \
        return hmpc::ints::num::FUNCTION(left, right); \
    }
#define HMPC_COMPARISON_OPERATORS(T) \
    HMPC_COMPARISON_OPERATOR(T, ==, equal_to) \
    HMPC_COMPARISON_OPERATOR(T, !=, not_equal_to)
#define HMPC_OPERATOR_BODY_ADD(TARGET, RESULT, LEFT, RIGHT) \
    TARGET; \
    hmpc::core::bit_array<bit_size + 1, limb_type, hmpc::without_sign> sum; \
    hmpc::ints::num::add(sum, LEFT, RIGHT); \
    if constexpr (sum.limb_size == limb_size) \
    { \
        auto underflow = hmpc::ints::num::subtract(RESULT, sum, modulus_span); \
        hmpc::ints::num::select(RESULT, RESULT, sum, underflow); \
        return RESULT; \
    } \
    else \
    { \
        static_assert(sum.limb_size == limb_size + 1); \
        hmpc::core::bit_array<bit_size + 1, limb_type, hmpc::without_sign> difference; \
        auto underflow = hmpc::ints::num::subtract(difference, sum, modulus_span); \
        hmpc::ints::num::select(RESULT, difference, sum, underflow); \
        return RESULT; \
    }
#define HMPC_OPERATOR_BODY_SUBTRACT(TARGET, RESULT, LEFT, RIGHT) \
    TARGET; \
    hmpc::core::bit_array<bit_size, limb_type, hmpc::without_sign> difference; \
    auto underflow = hmpc::ints::num::subtract(difference, LEFT, RIGHT); \
    hmpc::ints::num::add(RESULT, difference, modulus_span.mask(underflow)); \
    return RESULT;
#define HMPC_OPERATOR_BODY_MULTIPLY(TARGET, RESULT, LEFT, RIGHT) \
    TARGET; \
    hmpc::core::bit_array<bit_size + bit_size, limb_type, hmpc::without_sign> product; \
    hmpc::ints::num::multiply(product, LEFT, RIGHT); \
    hmpc::ints::num::montgomery_reduce(RESULT, product, modulus_span, hmpc::size_constant_of<limb_size>, inverse_modulus); \
    return RESULT;
#define HMPC_OPERATOR(T, OP, BODY) \
    friend constexpr T operator OP(T const& left, T const& right) HMPC_NOEXCEPT \
    { \
        BODY(T result, result, left, right) \
    }
#define HMPC_ASSIGNMENT_OPERATOR(T, OP, BODY) \
    constexpr T& operator OP##=(T const& other) HMPC_NOEXCEPT \
    { \
        BODY(, *this, *this, other) \
    }
#define HMPC_OPERATORS(T) \
    HMPC_OPERATOR(T, +, HMPC_OPERATOR_BODY_ADD) \
    HMPC_OPERATOR(T, -, HMPC_OPERATOR_BODY_SUBTRACT) \
    HMPC_OPERATOR(T, *, HMPC_OPERATOR_BODY_MULTIPLY)
#define HMPC_ASSIGNMENT_OPERATORS(T) \
    HMPC_ASSIGNMENT_OPERATOR(T, +, HMPC_OPERATOR_BODY_ADD) \
    HMPC_ASSIGNMENT_OPERATOR(T, -, HMPC_OPERATOR_BODY_SUBTRACT) \
    HMPC_ASSIGNMENT_OPERATOR(T, *, HMPC_OPERATOR_BODY_MULTIPLY)

namespace hmpc::ints
{
    struct from_uniformly_random_tag
    {
    };
    constexpr from_uniformly_random_tag from_uniformly_random = {};

    template<auto Modulus>
    struct mod
    {
        static constexpr auto modulus = Modulus;
        static constexpr auto modulus_constant = hmpc::constant_of<modulus>;
        static constexpr auto modulus_span = hmpc::core::constant_bit_span_from<modulus.span(hmpc::access::read)>;
        static constexpr auto half_modulus = modulus >> hmpc::constants::one;
        static constexpr auto half_modulus_constant = hmpc::constant_of<half_modulus>;
        static constexpr auto half_modulus_span = hmpc::core::constant_bit_span_from<half_modulus.span(hmpc::access::read)>;

        using limb_type = decltype(modulus)::limb_type;
        using normal_type = decltype(modulus)::normal_type;

        static_assert(hmpc::is_unsigned(modulus.signedness));
        static constexpr hmpc::signedness signedness = hmpc::without_sign;
        static_assert(modulus != hmpc::ints::zero<limb_type>);

        static constexpr hmpc::size bit_size = hmpc::ints::bit_width(modulus - hmpc::ints::one<limb_type>);

        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;
        static constexpr hmpc::size limb_size = hmpc::core::limb_size_for<limb_type>(bit_size);

        using unsigned_type = ubigint<bit_size, limb_type, normal_type>;
        using signed_type = sbigint<bit_size, limb_type, normal_type>;

        static constexpr auto inverse_modulus = []()
        {
            constexpr hmpc::ints::ubigint<limb_bit_size + 1, limb_type, normal_type> B = {0, 1};
            constexpr auto inverse = hmpc::ints::invert_modulo(modulus, B);
            return hmpc::constant_of<-inverse.data[0]>;
        }();

        static constexpr auto auxiliary_modulus = []()
        {
            hmpc::ints::ubigint<limb_size * limb_bit_size + 1, limb_type, normal_type> R = {};
            R.span(hmpc::access::write).write(hmpc::size_constant_of<limb_size>, hmpc::core::limb_traits<limb_type>::one, hmpc::access::unnormal);
            return R;
        }();

        static_assert(greatest_common_divisor(auxiliary_modulus, modulus) == hmpc::ints::one<limb_type>);

        static constexpr auto reduced_auxiliary_modulus = hmpc::ints::num::bit_copy<unsigned_type>(auxiliary_modulus % modulus);
        static constexpr auto reduced_auxiliary_modulus_span = hmpc::core::constant_bit_span_from<reduced_auxiliary_modulus.span(hmpc::access::read)>;

        static constexpr auto reduced_square_auxiliary_modulus = hmpc::ints::num::bit_copy<unsigned_type>((reduced_auxiliary_modulus * reduced_auxiliary_modulus) % modulus);
        static constexpr auto reduced_square_auxiliary_modulus_span = hmpc::core::constant_bit_span_from<reduced_square_auxiliary_modulus.span(hmpc::access::read)>;

        static constexpr auto reduced_cubed_auxiliary_modulus = []()
        {
            auto intermediate_result = reduced_square_auxiliary_modulus * reduced_square_auxiliary_modulus;
            unsigned_type result;
            hmpc::ints::num::montgomery_reduce(result, intermediate_result, modulus_span, hmpc::size_constant_of<limb_size>, inverse_modulus);
            return result;
        }();
        static constexpr auto reduced_cubed_auxiliary_modulus_span = hmpc::core::constant_bit_span_from<reduced_cubed_auxiliary_modulus.span(hmpc::access::read)>;

    private:
        template<hmpc::size OtherLimbSize>
        static constexpr auto reduced_other_auxiliary_modulus_times_square_auxiliary_modulus_span = []()
        {
            constexpr auto reduced_other_auxiliary_modulus_times_square_auxiliary_modulus = []()
            {
                constexpr auto other_auxiliary_modulus = []()
                {
                    hmpc::ints::ubigint<limb_bit_size * OtherLimbSize + 1, limb_type, normal_type> R = {};
                    R.span(hmpc::access::write).write(hmpc::size_constant_of<OtherLimbSize>, hmpc::core::limb_traits<limb_type>::one, hmpc::access::unnormal);
                    return R;
                }();

                return hmpc::ints::num::bit_copy<unsigned_type>((other_auxiliary_modulus * reduced_square_auxiliary_modulus) % modulus);
            }();
            return hmpc::core::constant_bit_span_of<reduced_other_auxiliary_modulus_times_square_auxiliary_modulus>;
        }();

        template<auto OtherModulus>
        static constexpr auto offset_to = []()
        {
            constexpr auto reduced_other_modulus = (OtherModulus % modulus);
            if constexpr (reduced_other_modulus == hmpc::ints::zero<limb_type>)
            {
                return hmpc::ints::zero<limb_type>;
            }
            else
            {
                return modulus - reduced_other_modulus;
            }
        }();

        /// Note, auxiliary_modulus == pow(2, limb_size * limb_bit_size).
        /// We can perform montgomery_reduce(x) for 0 <= x < auxiliary_modulus * modulus.
        /// For 0 <= value < pow(2, UnsignedIntegerLike::bit_size) <= auxiliary_modulus and x = value * reduced_square_auxiliary_modulus:
        /// 0 <= x < auxiliary_modulus * (modulus - 1) < auxiliary_modulus * modulus.
        template<typename UnsignedIntegerLike>
        constexpr void from_unsigned_integer(UnsignedIntegerLike const& value)
            requires (UnsignedIntegerLike::bit_size <= limb_size * limb_bit_size)
        {
            constexpr auto value_bit_size = UnsignedIntegerLike::bit_size;
            hmpc::core::bit_array<value_bit_size + bit_size, limb_type, hmpc::without_sign> intermediate;
            hmpc::ints::num::multiply(intermediate, value, reduced_square_auxiliary_modulus_span);
            hmpc::ints::num::montgomery_reduce(*this, intermediate, modulus_span, hmpc::size_constant_of<limb_size>, inverse_modulus);
        }

        /// We perform montgomery_reduce twice to first reduce the intermediate results to [0,modulus) and then adjust it to be in Montgomery form.
        /// Let R' = pow(2, other_integer_type::limb_size * limb_bit_size).
        /// The first reduction will give us y = (value * R'^{-1} mod modulus).
        /// Then, we compute
        /// montgomery_reduce(y * (R' * auxiliary_modulus^2 mod modulus)) == value * R'^{-1} * R' * auxiliary_modulus^2 * auxiliary_modulus^{-1} mod modulus
        template<typename UnsignedIntegerLike>
        constexpr void from_unsigned_integer(UnsignedIntegerLike const& value)
        {
            constexpr auto value_limb_size = UnsignedIntegerLike::limb_size;
            hmpc::ints::num::montgomery_reduce(*this, value, modulus_span, hmpc::size_constant_of<value_limb_size>, inverse_modulus);

            hmpc::core::bit_array<bit_size + bit_size, limb_type, hmpc::without_sign> intermediate;
            hmpc::ints::num::multiply(intermediate, *this, reduced_other_auxiliary_modulus_times_square_auxiliary_modulus_span<value_limb_size>);
            hmpc::ints::num::montgomery_reduce(*this, intermediate, modulus_span, hmpc::size_constant_of<limb_size>, inverse_modulus);
        }

    public:
        /// Data member
        hmpc::core::bit_array<bit_size, limb_type, signedness, normal_type> data;

        constexpr mod() HMPC_NOEXCEPT = default;

        /// # Constructor from unsigned integer
        /// We can compute the Montgomery form (value * auxiliary_modulus mod modulus) by computing
        /// montgomery_reduce(value * auxiliary_modulus^2) == value * auxiliary_modulus^2 * auxiliary_modulus^{-1} mod modulus
        template<hmpc::size Bits, typename OtherNormal>
        explicit constexpr mod(hmpc::ints::ubigint<Bits, limb_type, OtherNormal> const& value) HMPC_NOEXCEPT
        {
            from_unsigned_integer(value);
        }

        /// # Constructor from unsigned integer when generating a uniformly random mod
        /// This simply calls `hmpc::ints::num::montgomery_reduce`.
        /// For R' (based on other_integer_type::limb_size), the output will be (value * R'^{-1} mod modulus).
        /// As R' is invertible and value is uniformly random, the result is also uniformly random and we save multiple steps of multiplying and reducing.
        template<hmpc::size Bits, typename OtherNormal>
        explicit constexpr mod(hmpc::ints::ubigint<Bits, limb_type, OtherNormal> const& value, hmpc::ints::from_uniformly_random_tag) HMPC_NOEXCEPT
        {
            using other_integer_type = hmpc::ints::ubigint<Bits, limb_type, OtherNormal>;
            hmpc::ints::num::montgomery_reduce(*this, value, modulus_span, hmpc::size_constant_of<std::max(limb_size, other_integer_type::limb_size)>, inverse_modulus);
        }

        /// # Constructor from small signed integer
        /// First computes a (small) unsigned integer from value and then proceeds as for unsigned integers.
        template<hmpc::size Bits, typename OtherNormal>
            requires (Bits < bit_size)
        explicit constexpr mod(hmpc::ints::sbigint<Bits, limb_type, OtherNormal> const& value) HMPC_NOEXCEPT
        {
            auto read_value = value.span(hmpc::access::read);
            hmpc::core::num::add(span(hmpc::access::write), modulus_span.mask(read_value.sign()), read_value);
            from_unsigned_integer(*this);
        }

        /// # Constructor from larger signed integer
        /// Let largest_absolute_value = pow(2, Bits - 1)
        /// We know, -largest_absolute_value <= value < largest_absolute_value.
        /// Let M be the next multiple of modulus >= largest_absolute_value
        /// if 0 <= value < largest_absolute_value <= M:
        ///     Simply reduce value like a (large) unsigned integer in [0,M).
        /// if -largest_absolute_value <= value < 0:
        ///     Compute y = M + value and reduce y like a (large) unsigned integer in [0,M).
        template<hmpc::size Bits, typename OtherNormal>
        explicit constexpr mod(hmpc::ints::sbigint<Bits, limb_type, OtherNormal> const& value) HMPC_NOEXCEPT
        {
            using other_integer_type = hmpc::ints::sbigint<Bits, limb_type, OtherNormal>;
            constexpr auto other_unsigned_bit_size = other_integer_type::bit_size - 1;
            constexpr auto largest_absolute_value = hmpc::ints::one<limb_type> << hmpc::size_constant_of<other_unsigned_bit_size>; // == abs(other_integer_type::min) == 2^other_unsigned_bit_size
            constexpr auto next_modulus_multiple = largest_absolute_value + offset_to<largest_absolute_value>;
            constexpr auto next_modulus_multiple_span = hmpc::core::constant_bit_span_of<next_modulus_multiple>;
            constexpr auto max_value = next_modulus_multiple - hmpc::ints::one<limb_type>;
            constexpr auto normalized_integer_bit_size = bit_width(max_value);

            auto read_value = value.span(hmpc::access::read);

            hmpc::core::bit_array<normalized_integer_bit_size, limb_type, hmpc::without_sign> normalized_integer;
            hmpc::core::num::add(normalized_integer.span(hmpc::access::write), next_modulus_multiple_span.mask(read_value.sign()), read_value);

            from_unsigned_integer(normalized_integer);
        }

        /// # Constructor from other mod
        /// First converts value to a unsigned integer.
        /// Then, shifts the integer value depending on the sign (as if it was a signed integer).
        /// Finally, proceeds as for unsigned integers.
        template<auto OtherModulus>
        explicit constexpr mod(mod<OtherModulus> const& value) HMPC_NOEXCEPT
        {
            static_assert(OtherModulus != Modulus);
            using other_type = mod<OtherModulus>;
            static_assert(std::same_as<limb_type, typename other_type::limb_type>);

            auto integer_value = static_cast<other_type::unsigned_type>(value);

            if constexpr (modulus > other_type::modulus)
            {
                constexpr auto offset = modulus - other_type::modulus;
                constexpr auto offset_span = hmpc::core::constant_bit_span_of<offset>;

                hmpc::ints::num::add(
                    *this,
                    integer_value,
                    offset_span.mask(
                        hmpc::ints::num::greater(
                            integer_value,
                            other_type::half_modulus_span
                        )
                    )
                );

                from_unsigned_integer(*this);
            }
            else
            {
                constexpr auto offset = offset_to<other_type::modulus>;
                constexpr auto offset_span = hmpc::core::constant_bit_span_of<offset>;
                constexpr auto max_value = other_type::modulus + offset - hmpc::ints::one<limb_type>;
                constexpr auto normalized_integer_bit_size = bit_width(max_value);

                hmpc::core::bit_array<normalized_integer_bit_size, limb_type, hmpc::without_sign> normalized_integer;
                hmpc::ints::num::add(
                    normalized_integer,
                    integer_value,
                    offset_span.mask(
                        hmpc::ints::num::greater(
                            integer_value,
                            other_type::half_modulus_span
                        )
                    )
                );

                from_unsigned_integer(normalized_integer);
            }
        }

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

        HMPC_COMPARISON_OPERATORS(mod)

        HMPC_OPERATORS(mod)

        HMPC_ASSIGNMENT_OPERATORS(mod)

        constexpr mod operator-() const HMPC_NOEXCEPT
        {
            mod result;
            hmpc::ints::num::subtract(
                result,
                modulus_span.mask(
                    hmpc::ints::num::not_equal_to(
                        *this,
                        hmpc::core::nullspan<limb_type>
                    )
                ),
                *this
            );
            return result;
        }

        friend consteval mod invert(mod const& value)
        {
            unsigned_type greatest_common_divisor;
            unsigned_type inverse;
            hmpc::ints::num::extended_euclidean(greatest_common_divisor, inverse, hmpc::core::compiletime_nullspan<limb_type>, value, modulus);
            HMPC_COMPILETIME_ASSERT(greatest_common_divisor == hmpc::ints::one<limb_type>);
            hmpc::core::bit_array<bit_size + bit_size, limb_type, hmpc::without_sign> product;
            hmpc::ints::num::multiply(product, inverse, reduced_cubed_auxiliary_modulus_span);
            mod result;
            hmpc::ints::num::montgomery_reduce(result, product, modulus_span, hmpc::size_constant_of<limb_size>, inverse_modulus);
            return result;
        }

        explicit constexpr operator unsigned_type() const HMPC_NOEXCEPT
        {
            unsigned_type result;
            hmpc::ints::num::montgomery_reduce(result, *this, modulus_span, hmpc::size_constant_of<limb_size>, inverse_modulus);
            return result;
        }

        explicit constexpr operator signed_type() const HMPC_NOEXCEPT
        {
            auto unsigned_result = static_cast<unsigned_type>(*this);
            signed_type result;
            hmpc::ints::num::subtract(
                result,
                unsigned_result,
                modulus_span.mask(
                    hmpc::ints::num::greater(
                        unsigned_result,
                        half_modulus_span
                    )
                )
            );
            return result;
        }
    };

    template<auto Modulus, hmpc::size Exponent>
    constexpr auto pow(mod<Modulus> value, hmpc::size_constant<Exponent> exponent) HMPC_NOEXCEPT
    {
        static_assert(exponent >= 0);

        using mod = mod<Modulus>;
        using limb_type = mod::limb_type;

        if constexpr (exponent == 0)
        {
            return mod(hmpc::ints::one<limb_type>);
        }
        else
        {
            auto result = mod(hmpc::ints::one<limb_type>);

            hmpc::iter::for_range<hmpc::detail::bit_width(Exponent) - 1>([&](auto i)
            {
                if constexpr ((exponent >> i) & hmpc::constants::one)
                {
                    result *= value;
                }
                value *= value;
            });
            return value * result;
        }
    }

    template<auto Modulus, typename T>
        requires (hmpc::is_unsigned(T::signedness))
    consteval auto pow(mod<Modulus> value, T const& exponent)
    {
        using mod = mod<Modulus>;
        using limb_type = mod::limb_type;

        if (exponent == hmpc::ints::zero<limb_type>)
        {
            return mod(hmpc::ints::one<limb_type>);
        }

        auto result = mod(hmpc::ints::one<limb_type>);

        auto read = exponent.compiletime_span(hmpc::access::read);
        for (hmpc::size i = 0; i < T::bit_size; ++i)
        {
            if (hmpc::core::num::extract_bit(read, i))
            {
                result *= value;
            }
            value *= value;
        }

        return result;
    }
}

#undef HMPC_COMPARISON_OPERATOR
#undef HMPC_COMPARISON_OPERATORS
#undef HMPC_OPERATOR_BODY_ADD
#undef HMPC_OPERATOR_BODY_SUBTRACT
#undef HMPC_OPERATOR_BODY_MULTIPLY
#undef HMPC_OPERATOR
#undef HMPC_OPERATORS
#undef HMPC_ASSIGNMENT_OPERATOR
#undef HMPC_ASSIGNMENT_OPERATORS

template<auto Modulus, typename Char>
struct std::formatter<hmpc::ints::mod<Modulus>, Char>
{
    using type = hmpc::ints::mod<Modulus>;
    using big_integer_type = type::unsigned_type;
    std::formatter<big_integer_type, Char> underlying_formatter;

    static constexpr bool is_specialized = true;

    constexpr auto parse(auto& ctx)
    {
        return underlying_formatter.parse(ctx);
    }

    auto format(type const& value, auto& ctx) const
    {
        return underlying_formatter.format(static_cast<big_integer_type>(value), ctx);
    }
};
