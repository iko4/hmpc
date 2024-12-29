#pragma once

#include <type_traits>

namespace hmpc::typing
{
    template<typename T>
    concept reference = std::is_reference_v<T>;

    template<typename T>
    concept lvalue_reference = std::is_lvalue_reference_v<T>;

    template<typename T>
    concept rvalue_reference = std::is_rvalue_reference_v<T>;

    template<typename T>
    concept universal_reference_to_lvalue = lvalue_reference<T&&>;

    template<typename T>
    concept universal_reference_to_rvalue = rvalue_reference<T&&>;
}
