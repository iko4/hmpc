#pragma once

#include <hmpc/core/add.hpp>
#include <hmpc/core/masked_bit_span.hpp>
#include <hmpc/core/multiply.hpp>
#include <hmpc/core/num/compare.hpp>
#include <hmpc/iter/next.hpp>

namespace hmpc::core::num
{
    /// #### Algorithm reference
    /// - [1, MultiPrecisionREDC] Wikipedia Contributors: "Montgomery modular multiplication: Montgomery arithmetic on multiprecision integers." Wikipedia, 2023. [Link](https://en.wikipedia.org/wiki/Montgomery_modular_multiplication#Montgomery_arithmetic_on_multiprecision_integers), accessed 2023-08-31.
    /// - [2] Peter Montgomery: "Modular Multiplication Without Trial Division." Mathematics of Computation, Volume 44, Number 170, 1985. pp. 519-521. [Link](https://www.ams.oyrg/journals/mcom/1985-44-170/S0025-5718-1985-0777282-X/S0025-5718-1985-0777282-X.pdf), accessed 2023-08-31.
    ///
    /// #### Preconditions
    /// Let T = value
    /// Let N = modulus
    /// Let N' = inverse_modulus
    /// Let B = pow(2, limb_type::bit_size)
    /// Let r = auxiliary_power
    /// Let R = pow(B, r)
    /// Require gcd(R, N) == 1
    /// Require N * N' == -1 (mod B)
    /// Require 0 <= N' < B
    /// Require 0 <= T < R * N
    ///
    /// #### Postcondition
    /// Let S = result
    /// Require S = T * (R^{-1} mod N) (mod N)
    template<hmpc::unsigned_write_only_bit_span Result, hmpc::unsigned_read_only_bit_span Value, hmpc::unsigned_read_only_bit_span Modulus, hmpc::size AuxiliaryPower, hmpc::is_constant_of<typename Result::limb_type> InverseModulus>
        requires (hmpc::same_limb_types<Result, Value, Modulus>)
    constexpr void montgomery_reduce(Result result, Value value, Modulus modulus, hmpc::size_constant<AuxiliaryPower> auxiliary_power, InverseModulus inverse_modulus) HMPC_NOEXCEPT
    {
        using limb_type = Result::limb_type;
        using limb_traits = hmpc::core::limb_traits<limb_type>;
        constexpr auto r = AuxiliaryPower;
        constexpr auto p = modulus.limb_size;
        HMPC_DEVICE_ASSERT(hmpc::core::limb_size_for<limb_type>(bit_width(modulus)) == modulus.limb_size);

        hmpc::core::bit_array<limb_traits::bit_size * (r + p + 1), limb_type, hmpc::without_sign> T_storage = {};
        auto T = T_storage.span(hmpc::access::read_write);

        bit_copy(T_storage.span(hmpc::access::write), value);

        hmpc::iter::for_range<r>([&](auto i)
        {
            auto m = hmpc::core::multiply(T.read(i), inverse_modulus);
            auto carry = hmpc::iter::scan_range<p>([&](auto j, auto carry)
            {
                constexpr auto k = hmpc::iter::next(i, j);
                auto [lower, upper] = hmpc::core::extended_multiply_add(m, modulus.read(j), T.read(k), carry);
                T.write(k, lower);
                return upper;
            }, hmpc::zero_constant_of<limb_type>);
            hmpc::iter::scan_range<i + p, T.limb_size>([&](auto k, auto carry)
            {
                auto [sum, new_carry] = hmpc::core::extended_add(T.read(k), carry);
                T.write(k, sum);
                return new_carry;
            }, carry);
        });

        auto S = T_storage.span(hmpc::access::read).subspan(auxiliary_power);

        subtract(result, S, modulus.mask(greater_equal(S, modulus)));
    }
}
