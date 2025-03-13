#pragma once

#include <hmpc/core/bit_and.hpp>
#include <hmpc/core/bit_not.hpp>
#include <hmpc/core/bit_span.hpp>
#include <hmpc/core/equal_to.hpp>
#include <hmpc/core/greater.hpp>
#include <hmpc/core/greater_equal.hpp>
#include <hmpc/core/is_set.hpp>
#include <hmpc/core/less.hpp>
#include <hmpc/core/less_equal.hpp>
#include <hmpc/core/max.hpp>
#include <hmpc/core/not_equal_to.hpp>
#include <hmpc/core/select.hpp>
#include <hmpc/iter/scan_reverse_range.hpp>

#include <algorithm>

#define HMPC_COMPARE_EQUAL(NAME, DEFAULT, JOIN) \
    template<hmpc::read_only_bit_span Left, hmpc::read_only_bit_span Right> \
        requires (hmpc::same_limb_types<Left, Right>) \
    constexpr hmpc::bit NAME(Left left, Right right) HMPC_NOEXCEPT \
    { \
        constexpr auto limbs = hmpc::core::max(left.limb_size, right.limb_size); \
\
        auto initial_result = [&]() \
        { \
            if constexpr ((left.signedness != right.signedness) and (left.bit_size == right.bit_size) and (left.bit_size % left.limb_bit_size == 0)) \
            { \
                return hmpc::core::NAME(left.sign(), right.sign()); \
            } \
            else \
            { \
                return hmpc::constant_of<hmpc::bit{DEFAULT}>; \
            } \
        }(); \
        return hmpc::iter::scan(hmpc::range(limbs), [&](auto i, auto result) { return hmpc::core::JOIN(result, hmpc::core::NAME(left.extended_read(i, hmpc::access::normal), right.extended_read(i, hmpc::access::normal))); }, initial_result); \
    }


#define HMPC_COMPARE(NAME, NEGATIVE_NAME, DEFAULT) \
    template<hmpc::read_only_bit_span Left, hmpc::read_only_bit_span Right> \
        requires (hmpc::same_limb_types<Left, Right>) \
    constexpr hmpc::bit NAME(Left left, Right right) HMPC_NOEXCEPT \
    { \
        constexpr auto limbs = hmpc::core::max(left.limb_size, right.limb_size); \
\
        auto initial_result = [&]() \
        { \
            if constexpr (hmpc::is_unsigned(left.signedness) and hmpc::is_unsigned(right.signedness)) \
            { \
                return hmpc::core::compare_result{hmpc::constant_of<hmpc::bit{DEFAULT}>, hmpc::constants::bit::zero}; \
            } \
            else \
            { \
                /* op | left.sign | right.sign | result \
                / ----+-----------+------------+------- \
                /   < |         0 |          1 |      0 \
                /   < |         1 |          0 |      1 \
                /  <= |         0 |          1 |      0 \
                /  <= |         1 |          0 |      1 \
                /   > |         0 |          1 |      1 \
                /   > |         1 |          0 |      0 \
                /  >= |         0 |          1 |      1 \
                /  >= |         1 |          0 |      0 \
                / \
                /  -> initial_result = left.sign NEGATIVE_OP right.sign \
                /     initial_determined = left.sign != right.sign \
                */ \
                return hmpc::core::compare_result{hmpc::core::NEGATIVE_NAME(left.sign(), right.sign()), hmpc::core::not_equal_to(left.sign(), right.sign())}; \
            } \
        }(); \
\
        return hmpc::iter::scan_reverse(hmpc::range(limbs), [&](auto i, auto result) \
        { \
            /* result determined | previous result | comparison | result determined | new result  \
            /  ------------------+-----------------+------------+-------------------+-----------  \
            /                  x |               y |          = |                 x |          y  \
            /                  0 |               y |          z |                 1 |          z  \
            /                  1 |               y |          z |                 1 |          y  \
            /                                                                                     \
            / -> equal = (left == right)                                                          \
            /    result = select(result, left op right, not result_determined and not equal)      \
            /    result_determined |= not equal                                                   \
            */\
            auto x = left.extended_read(i, hmpc::access::normal);\
            auto y = right.extended_read(i, hmpc::access::normal);\
            \
            auto not_equal = hmpc::core::not_equal_to(x, y);\
            auto now_determined = hmpc::core::bit_or(result.determined, not_equal);\
            return\
                hmpc::core::compare_result{\
                    hmpc::core::select(\
                        result.result,\
                        hmpc::core::NAME(x, y),\
                        hmpc::core::bit_and(hmpc::core::bit_not(result.determined), not_equal)\
                    ),\
                    now_determined\
                }; \
        }, initial_result).result; \
    }

#define HMPC_COMPILETIME_COMPARE_EQUAL(NAME, DEFAULT) \
    template<hmpc::read_only_compiletime_bit_span Left, hmpc::read_only_compiletime_bit_span Right> \
        requires (hmpc::same_limb_types<Left, Right>) \
    consteval hmpc::bit NAME(Left left, Right right) \
    { \
        hmpc::size limbs = std::max(left.limb_size, right.limb_size); \
        for (hmpc::size i = 0; i < limbs; ++i) \
        { \
            if (left.extended_read(i, hmpc::access::normal) != right.extended_read(i, hmpc::access::normal)) \
            { \
                return hmpc::bit{not DEFAULT}; \
            } \
        } \
        return hmpc::bit{DEFAULT}; \
    }

#define HMPC_COMPILETIME_COMPARE(NAME, OP, DEFAULT) \
    template<hmpc::read_only_compiletime_bit_span Left, hmpc::read_only_compiletime_bit_span Right> \
        requires (hmpc::same_limb_types<Left, Right>) \
    consteval hmpc::bit NAME(Left left, Right right) \
    { \
        hmpc::size limbs = std::max(left.limb_size, right.limb_size); \
        for (hmpc::size i = limbs; i-- > 0;) \
        { \
            auto x = left.extended_read(i, hmpc::access::normal); \
            auto y = right.extended_read(i, hmpc::access::normal); \
            if (x != y) \
            { \
                return x OP y; \
            } \
        } \
        return hmpc::bit{DEFAULT}; \
    }

namespace hmpc::core
{
    template<typename Result, typename Determined>
    struct compare_result
    {
        Result result;
        Determined determined;

        constexpr compare_result(Result result, Determined determined) HMPC_NOEXCEPT
            : result(result)
            , determined(determined)
        {
        }
    };
}

namespace hmpc::core::num
{
    HMPC_COMPARE_EQUAL(equal_to, true, bit_and)
    HMPC_COMPARE_EQUAL(not_equal_to, false, bit_or)

    HMPC_COMPARE(greater, less, false)
    HMPC_COMPARE(less, greater, false)
    HMPC_COMPARE(greater_equal, less_equal, true)
    HMPC_COMPARE(less_equal, greater_equal, true)

    HMPC_COMPILETIME_COMPARE_EQUAL(equal_to, true)
    HMPC_COMPILETIME_COMPARE_EQUAL(not_equal_to, false)

    HMPC_COMPILETIME_COMPARE(greater, >, false)
    HMPC_COMPILETIME_COMPARE(less, <, false)
    HMPC_COMPILETIME_COMPARE(greater_equal, >=, true)
    HMPC_COMPILETIME_COMPARE(less_equal, <=, true)
}

#undef HMPC_COMPARE_EQUAL
#undef HMPC_COMPARE
#undef HMPC_COMPILETIME_COMPARE_EQUAL
#undef HMPC_COMPILETIME_COMPARE
