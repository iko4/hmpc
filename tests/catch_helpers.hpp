#pragma once

#include <hmpc/config.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

template<typename T>
    requires requires(T&& value)
    {
        std::format("{}", value);
        std::formatter<T, char>::is_specialized;
    }
struct Catch::StringMaker<T>
{
    static std::string convert(T const& value)
    {
        return std::format("{}", value);
    }
};
