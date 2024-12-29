#pragma once

#include <hmpc/config.hpp>

#include <bit>

namespace hmpc::detail
{
    template<typename T>
    struct type_id
    {
        static inline int dummy = 0;

        /// Returns an unique value for this type
        /// # Notes
        /// We cannot convert a pointer to a value at compile time (to constant initialize a variable)
        /// but we also cannot convert pointers to different objects.
        /// Therefore, this is a runtime cast.
        static constexpr hmpc::size value() noexcept
        {
            return std::bit_cast<hmpc::size>(&dummy);
        }
    };

    template<typename T>
    struct type_id<T&> : public type_id<T> {};
    template<typename T>
    struct type_id<T&&> : public type_id<T> {};
    template<typename T>
    struct type_id<T const> : public type_id<T> {};
    template<typename T>
    struct type_id<T const&> : public type_id<T> {};
    template<typename T>
    struct type_id<T const&&> : public type_id<T> {};

    template<typename T>
    constexpr auto type_id_of() noexcept
    {
        return type_id<T>::value();
    }
}
