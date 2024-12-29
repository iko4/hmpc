#pragma once

#include <hmpc/core/add.hpp>
#include <hmpc/core/bit_array.hpp>
#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/compiletime_bit_array.hpp>
#include <hmpc/core/compiletime_bit_span.hpp>
#include <hmpc/core/multiply.hpp>
#include <hmpc/iter/for_range.hpp>
#include <hmpc/iter/next.hpp>
#include <hmpc/iter/scan_range.hpp>

#include <algorithm>

namespace hmpc::core::num
{
    template<hmpc::unsigned_write_only_bit_span Result, hmpc::unsigned_read_only_bit_span Left, hmpc::unsigned_read_only_bit_span Right>
        requires (hmpc::same_limb_types<Result, Left, Right>)
    constexpr void multiply(Result result, Left left, Right right) HMPC_NOEXCEPT
    {
        using limb_type = Result::limb_type;
        hmpc::core::bit_array<std::min(result.bit_size, left.bit_size + right.bit_size), limb_type, result.signedness> intermediate_storage;
        auto intermediate = intermediate_storage.span();

        // left = a_{n-1} a_{n-2} ... a_1 a_0
        // right = b_{m-1} b_{m-2} ... b_1 b_0

        // left * right = \sum_{i=0}^{m-1} left * b_i * 2^{i * limb_bit_size}

        // left * b_i <= (2^{n * limb_bit_size} - 1) * (2^{limb_bit_size} - 1)
        //             = 2^{n * limb_bit_size + limb_bit_size} - 2^{n * limb_bit_size} - 2^{limb_bit_size} + 1
        //       = c_i = c_{i, n} c_{i, n-1} c_{i, n-2} ... c_{i, 1} c_{i, 0}

        // c_0 + c_1 * 2^{limb_bit_size} <= 2^{n * limb_bit_size + limb_bit_size} - 2^{n * limb_bit_size} - 2^{limb_bit_size} + 1 + 2^{n * limb_bit_size + 2 * limb_bit_size} - 2^{n * limb_bit_size + limb_bit_size} - 2^{limb_bit_size + limb_bit_size} + 2^{limb_bit_size}
        //                                = 2^{n * limb_bit_size + 2 * limb_bit_size} - 2^{n * limb_bit_size} - 2^{limb_bit_size + limb_bit_size} + 1

        // c_0 + c_1 * 2^{limb_bit_size} + c_2 * 2^{2 * limb_bit_size} <= 2^{n * limb_bit_size + 2 * limb_bit_size} - 2^{n * limb_bit_size} - 2^{limb_bit_size + limb_bit_size} + 1 + 2^{n * limb_bit_size + 3 * limb_bit_size} - 2^{n * limb_bit_size + 2 * limb_bit_size} - 2^{3 * limb_bit_size} + 2^{2 * limb_bit_size}
        //                                                              = 2^{n * limb_bit_size + 3 * limb_bit_size} - 2^{n * limb_bit_size} - 2^{3 * limb_bit_size} + 1

        // Hypothesis: \sum_{i=0}^{k} c_i * 2^{i * limb_bit_size} <= 2^{n * limb_bit_size + (k + 1) * limb_bit_size} - 2^{n * limb_bit_size} - 2^{(k + 1) * limb_bit_size} + 1
        // Induction: holds for 0
        // Induction: k -> k + 1
        // \sum_{i=0}^{k+1} c_i * 2^{i * limb_bit_size} <= 2^{n * limb_bit_size + (k + 1) * limb_bit_size} - 2^{n * limb_bit_size} - 2^{(k + 1) * limb_bit_size} + 1 + (2^{n * limb_bit_size + limb_bit_size} - 2^{n * limb_bit_size} - 2^{limb_bit_size} + 1) * 2^{(k + 1) * limb_bit_size}
        // = 2^{n * limb_bit_size + (k + 1) * limb_bit_size} - 2^{n * limb_bit_size} - 2^{(k + 1) * limb_bit_size} + 1 + 2^{n * limb_bit_size + limb_bit_size + (k + 1) * limb_bit_size} - 2^{n * limb_bit_size + (k + 1) * limb_bit_size} - 2^{limb_bit_size + (k + 1) * limb_bit_size} + 2^{(k + 1) * limb_bit_size}
        // = 2^{n * limb_bit_size + (k + 2) * limb_bit_size} - 2^{n * limb_bit_size} - 2^{(k + 2) * limb_bit_size} + 1
        // => holds

        // Implies: left * right <= 2^{n * limb_bit_size + m * limb_bit_size} - 2^{n * limb_bit_size} - 2^{m * limb_bit_size} + 1

        constexpr hmpc::size right_overlap = std::min(intermediate.limb_size, right.limb_size);
        hmpc::iter::for_range<right_overlap>([&](auto i)
        {
            static_assert(intermediate.limb_size >= i);
            constexpr hmpc::size left_overlap = std::min(intermediate.limb_size - i, left.limb_size);
            auto carry = hmpc::iter::scan_range<left_overlap>([&](auto j, auto carry)
            {
                auto left_value = left.read(j);
                auto right_value = right.read(i);

                constexpr auto k = hmpc::iter::next(i, j);

                auto current = [&]()
                {
                    if constexpr (i == 0)
                    {
                        return hmpc::zero_constant_of<limb_type>;
                    }
                    else
                    {
                        return intermediate.read(k);
                    }
                }();

                if constexpr (hmpc::iter::next(k) < result.limb_size)
                {
                    auto [lower, upper] = hmpc::core::extended_multiply_add(left_value, right_value, current, carry);
                    intermediate.write(k, lower);
                    return upper;
                }
                else
                {
                    static_assert(k == result.limb_size - 1);
                    intermediate.write(k, hmpc::core::multiply_add(left_value, right_value, current, carry));
                    return hmpc::zero_constant_of<limb_type>;
                }
            }, hmpc::zero_constant_of<limb_type>);
            if constexpr (intermediate.limb_size > left.limb_size + i)
            {
                intermediate.write(hmpc::iter::next(i, hmpc::size_constant_of<left.limb_size>), carry);
            }
        });

        hmpc::iter::for_range<intermediate.limb_size>([&](auto i)
        {
            result.write(i, intermediate.read(i));
        });
        hmpc::iter::for_range<intermediate.limb_size, result.limb_size>([&](auto i)
        {
            result.write(i, {}, hmpc::access::unnormal);
        });
    }

    template<hmpc::write_only_compiletime_bit_span Result, hmpc::read_only_compiletime_bit_span Left, hmpc::read_only_compiletime_bit_span Right>
        requires (hmpc::same_limb_types<Result, Left, Right>)
    consteval void multiply(Result result, Left left, Right right)
    {
        using limb_type = Result::limb_type;
        hmpc::core::compiletime_bit_array<limb_type> intermediate_storage(std::min(result.bit_size, left.bit_size + right.bit_size));
        auto intermediate = intermediate_storage.compiletime_span(hmpc::access::read_write);

        hmpc::size right_overlap = std::min(intermediate.limb_size, right.limb_size);
        for (hmpc::size i = 0; i < right_overlap; ++i)
        {
            HMPC_COMPILETIME_ASSERT(intermediate.limb_size >= i);
            hmpc::size left_overlap = std::min(intermediate.limb_size - i, left.limb_size);
            limb_type carry = {};
            for (hmpc::size j = 0; j < left_overlap; ++j)
            {
                auto left_value = left.read(j);
                auto right_value = right.read(i);

                auto k = i + j;

                auto current = (i == 0) ? limb_type{} : intermediate.read(k);

                if (k + 1 < result.limb_size)
                {
                    auto [lower, upper] = hmpc::core::extended_multiply_add(left_value, right_value, current, carry);
                    intermediate.write(k, lower);
                    carry = upper;
                }
                else
                {
                    HMPC_COMPILETIME_ASSERT(k == result.limb_size - 1);
                    intermediate.write(k, hmpc::core::multiply_add(left_value, right_value, current, carry));
                    carry = limb_type{};
                }
            }
            if (intermediate.limb_size > left.limb_size + i)
            {
                intermediate.write(i + left.limb_size, carry);
            }
        }

        for (hmpc::size i = 0; i < intermediate.limb_size; ++i)
        {
            result.write(i, intermediate.read(i));
        }
        for (hmpc::size i = intermediate.limb_size; i < result.limb_size; ++i)
        {
            result.write(i, hmpc::zero_constant_of<limb_type>, hmpc::access::unnormal);
        }
    }
}
