#pragma once

#include <hmpc/core/num/add.hpp>
#include <hmpc/core/num/bit_copy.hpp>
#include <hmpc/core/num/compare.hpp>
#include <hmpc/core/num/countr_zero.hpp>
#include <hmpc/core/num/divide.hpp>
#include <hmpc/core/num/multiply.hpp>
#include <hmpc/core/num/shift_left.hpp>
#include <hmpc/core/num/shift_right.hpp>
#include <hmpc/core/num/subtract.hpp>

namespace hmpc::core::num
{
    /// #### Algorithm reference
    /// - [1, gcd] Wikipedia Contributors: "Binary GCD algorithm: Implementation." Wikipedia, 2023. [Link](https://en.wikipedia.org/wiki/Binary_GCD_algorithm#Implementation), accessed 2023-08-31.
    template<hmpc::write_only_compiletime_bit_span Result, hmpc::read_only_compiletime_bit_span Left, hmpc::read_only_compiletime_bit_span Right>
        requires (hmpc::same_limb_types<Result, Left, Right>)
    consteval void greatest_common_divisor(Result result, Left left, Right right)
    {
        using limb_type = Result::limb_type;
        if (hmpc::core::num::equal_to(left, hmpc::core::read_compiletime_nullspan<limb_type>))
        {
            bit_copy(result, right);
        }
        else if (hmpc::core::num::equal_to(right, hmpc::core::read_compiletime_nullspan<limb_type>))
        {
            bit_copy(result, left);
        }
        else
        {
            auto width = std::max(left.bit_size, right.bit_size);
            hmpc::core::compiletime_bit_array<limb_type, hmpc::access::normal_tag> u_buffer(width);
            auto u_read = u_buffer.compiletime_span(hmpc::access::read);
            auto u_write = u_buffer.compiletime_span(hmpc::access::write);
            hmpc::core::compiletime_bit_array<limb_type, hmpc::access::normal_tag> v_buffer(width);
            auto v_read = v_buffer.compiletime_span(hmpc::access::read);
            auto v_write = v_buffer.compiletime_span(hmpc::access::write);

            auto left_power_of_two = countr_zero(left);
            auto right_power_of_two = countr_zero(right);
            auto common_power_of_two = std::min(left_power_of_two, right_power_of_two);

            shift_right(u_write, left, left_power_of_two);
            shift_right(v_write, right, right_power_of_two);

            while (hmpc::core::num::not_equal_to(u_read, v_read))
            {
                if (hmpc::core::num::less(u_read, v_read))
                {
                    std::swap(u_read, v_read);
                    std::swap(u_write, v_write);
                }

                subtract(u_write, u_read, v_read);
                shift_right(u_write, u_read, countr_zero(u_read));
            }

            shift_left(result, u_read, common_power_of_two);
        }
    }

    /// #### Algorithm reference
    /// - [1, extended_gcd] Wikipedia Contributors: "Extended Euclidean algorithm: Pseudocode." Wikipedia, 2023. [Link](https://en.wikipedia.org/wiki/Extended_Euclidean_algorithm#Pseudocode), accessed 2023-08-31.
    /// - [2] Stack Overflow Contributors: "Is it possible to implement the extended euclidean algorithm with unsigned machine words?" Stack Overflow, 2021. [Link](https://stackoverflow.com/q/67097428), accessed 2023-08-31.
    template<hmpc::write_only_compiletime_bit_span Result, hmpc::write_only_compiletime_bit_span LeftResult, hmpc::write_only_compiletime_bit_span RightResult, hmpc::read_only_compiletime_bit_span Left, hmpc::read_only_compiletime_bit_span Right>
        requires (hmpc::same_limb_types<Result, LeftResult, RightResult, Left, Right>)
    consteval void extended_euclidean(Result result, LeftResult left_result, RightResult right_result, Left left, Right right)
    {
        using limb_type = Result::limb_type;
        auto width = std::max(left.bit_size, right.bit_size);

        hmpc::core::compiletime_bit_array<limb_type> old_r_storage(width);
        auto read_old_r = old_r_storage.compiletime_span(hmpc::access::read);
        auto write_old_r = old_r_storage.compiletime_span(hmpc::access::write);
        hmpc::core::compiletime_bit_array<limb_type> r_storage(width);
        auto read_r = r_storage.compiletime_span(hmpc::access::read);
        auto write_r = r_storage.compiletime_span(hmpc::access::write);
        hmpc::core::compiletime_bit_array<limb_type> old_s_storage(width, 1);
        auto read_old_s = old_s_storage.compiletime_span(hmpc::access::read);
        auto write_old_s = old_s_storage.compiletime_span(hmpc::access::write);
        hmpc::core::compiletime_bit_array<limb_type> s_storage(width);
        auto read_s = s_storage.compiletime_span(hmpc::access::read);
        auto write_s = s_storage.compiletime_span(hmpc::access::write);
        hmpc::core::compiletime_bit_array<limb_type> old_t_storage(width);
        auto read_old_t = old_t_storage.compiletime_span(hmpc::access::read);
        auto write_old_t = old_t_storage.compiletime_span(hmpc::access::write);
        hmpc::core::compiletime_bit_array<limb_type> t_storage(width, 1);
        auto read_t = t_storage.compiletime_span(hmpc::access::read);
        auto write_t = t_storage.compiletime_span(hmpc::access::write);
        hmpc::core::compiletime_bit_array<limb_type> q_storage(width);
        auto read_q = q_storage.compiletime_span(hmpc::access::read);
        auto write_q = q_storage.compiletime_span(hmpc::access::write);
        hmpc::core::compiletime_bit_array<limb_type> tmp_storage(width);
        auto read_tmp = tmp_storage.compiletime_span(hmpc::access::read);
        auto write_tmp = tmp_storage.compiletime_span(hmpc::access::write);

        bit_copy(write_old_r, left);
        bit_copy(write_r, right);

        hmpc::bit iteration = {};
        while (hmpc::core::num::not_equal_to(read_r, hmpc::core::read_compiletime_nullspan<limb_type>))
        {
            divide(write_q, write_old_r, read_old_r, read_r);
            std::swap(read_old_r, read_r);
            std::swap(write_old_r, write_r);

            multiply(write_tmp, read_q, read_s);
            add(write_old_s, read_old_s, read_tmp);
            std::swap(read_old_s, read_s);
            std::swap(write_old_s, write_s);

            multiply(write_tmp, read_q, read_t);
            add(write_old_t, read_old_t, read_tmp);
            std::swap(read_old_t, read_t);
            std::swap(write_old_t, write_t);

            iteration = not iteration;
        }
        if (iteration)
        {
            subtract(write_old_s, right, read_old_s);
        }
        else
        {
            subtract(write_old_t, left, read_old_t);
        }
        bit_copy(result, read_old_r);
        bit_copy(left_result, read_old_s);
        bit_copy(right_result, read_old_t);
    }
}
