#pragma once

#include <cstdint>
#include <span>
#include <type_traits>
#include <variant>


#ifndef HMPC_ASSERT_LEVEL
    #ifdef HMPC_TESTING
        #define HMPC_ASSERT_LEVEL 2
    #else
        #define HMPC_ASSERT_LEVEL 0
    #endif
#endif

#if HMPC_ASSERT_LEVEL > 0
    #include <format>
    #include <source_location>
    #include <stdexcept>
    #define HMPC_NOEXCEPT
    #define HMPC_ASSERT(check) (hmpc::bool_cast(check)) ? static_cast<void>(0) : []() { constexpr auto location = std::source_location::current(); throw std::runtime_error(std::format("{}:{}:\n" "  Assertion failed: {}\n  in function: {}", location.file_name(), location.line(), #check, location.function_name())); }()
#else
    #define HMPC_NOEXCEPT noexcept(true)
    #define HMPC_ASSERT(...) static_cast<void>(0)
#endif
#define HMPC_COMPILETIME_ASSERT(CHECK) hmpc::compiletime_assert(CHECK)
#define HMPC_PRIVATE_MEMBER(NAME) hmpc_private_member_##NAME

#if HMPC_ASSERT_LEVEL > 0
    #define HMPC_HOST_ASSERT(check) HMPC_ASSERT(check)
    #define HMPC_HOST_NOEXCEPT
#else
    #define HMPC_HOST_ASSERT(check) static_cast<void>(0)
    #define HMPC_HOST_NOEXCEPT noexcept(true)
#endif

#if HMPC_ASSERT_LEVEL > 1
    #define HMPC_DEVICE_ASSERT(check) assert(hmpc::bool_cast(check) && #check)
    #define HMPC_DEVICE_NOEXCEPT
#else
    #define HMPC_DEVICE_ASSERT(check) static_cast<void>(0)
    #define HMPC_DEVICE_NOEXCEPT noexcept(true)
#endif

namespace hmpc
{
    struct compiletime_tag
    {
    };
    constexpr compiletime_tag compiletime = {};

    using monostate = std::monostate;
    constexpr monostate empty = {};

    using size = std::size_t;
    using signed_size = std::make_signed_t<size>;
    constexpr size dynamic_extent = std::dynamic_extent;
    constexpr size placeholder_extent = 0;

    template<typename T, T V>
    using constant = std::integral_constant<T, V>;

    namespace core
    {
        template<typename Underlying>
        struct uint;
    }

    using default_limb = core::uint<std::uint32_t>;
    using bit = core::uint<bool>;

    constexpr bool bool_cast(bit value) noexcept;
    constexpr bool bool_cast(bool value) noexcept
    {
        return value;
    }

    namespace crypto
    {
        template<hmpc::size Rounds, hmpc::size NonceSize, hmpc::size CounterSize>
        struct chacha;

        using chacha20 = chacha<20, 3, 1>;
    }

    using default_random_engine = crypto::chacha20;

    enum class rotate : size
    {
    };

    enum class statistical_security : size
    {
    };
    enum class computational_security : size
    {
    };

    constexpr auto default_statistical_security = statistical_security{80};
    constexpr auto default_computational_security = computational_security{128};
}
