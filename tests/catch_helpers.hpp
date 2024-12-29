#pragma once

#include <hmpc/config.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

template<typename T>
    requires requires(T&& value)
    {
        HMPC_FMTLIB::format("{}", value);
        HMPC_FMTLIB::formatter<T, char>::is_specialized;
    }
struct Catch::StringMaker<T>
{
    static std::string convert(T const& value)
    {
        return HMPC_FMTLIB::format("{}", value);
    }
};
