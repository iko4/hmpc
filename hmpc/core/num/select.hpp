#pragma once

#include <hmpc/core/limb_span.hpp>
#include <hmpc/core/select.hpp>
#include <hmpc/iter/for_each_range.hpp>

namespace hmpc::core::num
{
    template<hmpc::write_only_limb_span Result, hmpc::read_only_limb_span FalseValue, hmpc::read_only_limb_span TrueValue, typename Choice>
        requires (hmpc::same_limb_types<Result, FalseValue, TrueValue> and hmpc::maybe_constant_of<Choice, hmpc::bit>)
    constexpr void select(Result result, FalseValue false_value, TrueValue true_value, Choice choice) HMPC_NOEXCEPT
    {
        hmpc::iter::for_each(hmpc::range(result.limb_size), [&](auto i)
        {
            result.write(i, hmpc::core::select(false_value.extended_read(i), true_value.extended_read(i), choice));
        });
    }
}
