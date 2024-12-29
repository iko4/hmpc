#pragma once

#include <hmpc/core/bit_span.hpp>
#include <hmpc/iter/for_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::write_only_bit_span Result, hmpc::read_only_bit_span Value>
        requires (Result::bit_size == Value::bit_size)
    void convert(Result result, Value value) HMPC_NOEXCEPT
    {
        using result_limb_type = Result::limb_type;
        if constexpr (result.limb_bit_size < value.limb_bit_size)
        {
            static_assert(value.limb_bit_size % result.limb_bit_size == 0);
            constexpr hmpc::size limb_ratio = value.limb_bit_size / result.limb_bit_size;
            constexpr bool has_partial_limbs = result.limb_size * limb_ratio != value.limb_size;
            constexpr hmpc::size full_limbs = value.limb_size - static_cast<hmpc::size>(has_partial_limbs);
            hmpc::iter::for_range<full_limbs>([&](auto i)
            {
                auto value_limb = value.read(i);
                result.write(hmpc::size_constant_of<i * limb_ratio>, result_limb_type{value_limb});
                for (hmpc::size j = 1; j < limb_ratio; ++j)
                {
                    value_limb >>= hmpc::size_constant_of<result.limb_bit_size>;
                    result.write(hmpc::size_constant_of<i * limb_ratio + j>, result_limb_type{value_limb});
                }
            });
            if constexpr (has_partial_limbs)
            {
                static_assert(result.limb_size > full_limbs * limb_ratio);
                constexpr hmpc::size final_partial_limbs = result.limb_size - full_limbs * limb_ratio;
                auto value_limb = value.read(hmpc::size_constant_of<full_limbs>);
                result.write(hmpc::size_constant_of<full_limbs * limb_ratio>, result_limb_type{value_limb});
                hmpc::iter::for_range<hmpc::size{1}, final_partial_limbs>([&](auto j)
                {
                    value_limb >>= hmpc::size_constant_of<result.limb_bit_size>;
                    result.write(hmpc::size_constant_of<full_limbs * limb_ratio + j>, result_limb_type{value_limb});
                });
            }
        }
        else if constexpr (result.limb_bit_size > value.limb_bit_size)
        {
            static_assert(result.limb_bit_size % value.limb_bit_size == 0);
            constexpr hmpc::size limb_ratio = result.limb_bit_size / value.limb_bit_size;
            constexpr bool has_partial_limbs = value.limb_size * limb_ratio != result.limb_size;
            constexpr hmpc::size full_limbs = result.limb_size - static_cast<hmpc::size>(has_partial_limbs);
            hmpc::iter::for_range<full_limbs>([&](auto i)
            {
                auto result_limb = result_limb_type{value.read(hmpc::size_constant_of<i * limb_ratio>)};
                hmpc::iter::for_range<hmpc::size{1}, limb_ratio>([&](auto j)
                {
                    auto value_limb = result_limb_type{value.read(hmpc::size_constant_of<i * limb_ratio + j>)};
                    value_limb <<= hmpc::size_constant_of<j * value.limb_bit_size>;
                    result_limb |= value_limb;
                });
                result.write(i, result_limb);
            });
            if constexpr (has_partial_limbs)
            {
                static_assert(value.limb_size > full_limbs * limb_ratio);
                constexpr hmpc::size final_partial_limbs = value.limb_size - full_limbs * limb_ratio;
                auto result_limb = result_limb_type{value.read(hmpc::size_constant_of<full_limbs * limb_ratio>)};
                hmpc::iter::for_range<hmpc::size{1}, final_partial_limbs>([&](auto j)
                {
                    auto value_limb = result_limb_type{value.read(hmpc::size_constant_of<full_limbs * limb_ratio + j>)};
                    value_limb <<= hmpc::size_constant_of<j * value.limb_bit_size>;
                    result_limb |= value_limb;
                });
                result.write(hmpc::size_constant_of<full_limbs>, result_limb);
            }
        }
        else
        {
            static_assert(result.limb_size == value.limb_size);
            hmpc::iter::for_range<result.limb_size>([&](auto i)
            {
                result.write(i, value.read(i));
            });
        }
    }
}
