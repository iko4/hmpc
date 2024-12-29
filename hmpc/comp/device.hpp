#pragma once

#include <hmpc/config.hpp>
#include <hmpc/fmt.hpp>

namespace hmpc::comp
{
    enum class device_type
    {
        cpu,
        gpu,
        unknown,
    };

    struct device_limits
    {
        std::uint32_t compute_units;
        std::uint32_t sub_devices;
        std::uint32_t work_item_dimensions;
        std::size_t work_item_size_1d;
        std::array<std::size_t, 2> work_item_size_2d;
        std::array<std::size_t, 3> work_item_size_3d;
        std::size_t work_group_size;
        std::uint32_t sub_groups;
        std::size_t parameter_size;
        std::uint64_t global_memory_size;
        std::uint64_t global_memory_cache_size;
        std::uint32_t global_memory_cache_line_size;
        std::uint64_t local_memory_size;
    };

    struct device_info
    {
        device_type type;
        std::string name;
        std::string vendor;
        std::string driver;
        bool available;
        std::uint32_t address_bits;
        std::uint32_t address_align_bits;
        device_limits limits;
    };
}

/// Format hmpc::comp::device_limits
///
/// The format specifier is
/// ```
/// n(optional) h(optional) (u|U)(optional)
/// ```
/// where
/// - 'n': omits outer braces
/// - 'h': converts the numbers to nice "human-readable" representations
/// - 'u': adds units ("b" for bits)
/// - 'U': adds units ("B" for bytes)
template<typename Char>
struct HMPC_FMTLIB::formatter<hmpc::comp::device_limits, Char>
{
    static constexpr bool is_specialized = true;

    bool no_braces = false;
    bool human_readable = false;
    hmpc::fmt::units::binary units = hmpc::fmt::units::binary::none;

    constexpr auto parse(auto& ctx)
    {
        using hmpc::fmt::units::binary;

        auto it = ctx.begin();
        if (it == ctx.end())
        {
            return it;
        }

        if (*it == Char{'n'})
        {
            no_braces = true;
            ++it;
        }
        if (it != ctx.end() and *it == Char{'h'})
        {
            human_readable = true;
            ++it;
        }
        if (it != ctx.end() and *it == Char{'u'})
        {
            units = binary::bit;
            ++it;
        }
        else if (it != ctx.end() and *it == Char{'U'})
        {
            units = binary::byte;
            ++it;
        }
        if (*it != Char{'}'})
        {
            throw HMPC_FMTLIB::format_error("Invalid format specifier");
        }

        return it;
    }

    auto format(hmpc::comp::device_limits const& limits, auto& ctx) const
    {
        auto out = ctx.out();
        if (not no_braces)
        {
            out = HMPC_FMTLIB::format_to(out, "{{");
        }

        out = HMPC_FMTLIB::format_to(out, "compute units: {}, sub devices: {}, work item dimensions: {}, work item size: {} or ({}, {}) or ({}, {}, {}), work group size: {}, sub groups: {}, parameter size: ", limits.compute_units, limits.sub_devices, limits.work_item_dimensions, limits.work_item_size_1d, limits.work_item_size_2d[0], limits.work_item_size_2d[1], limits.work_item_size_3d[0], limits.work_item_size_3d[1], limits.work_item_size_3d[2], limits.work_group_size, limits.sub_groups);
        out = hmpc::fmt::format_maybe_human_readable_byte_size(out, human_readable, limits.parameter_size, units);
        out = HMPC_FMTLIB::format_to(out, ", global memory size: ");
        out = hmpc::fmt::format_maybe_human_readable_byte_size(out, human_readable, limits.global_memory_size, units);
        out = HMPC_FMTLIB::format_to(out, ", global memory cache size: ");
        out = hmpc::fmt::format_maybe_human_readable_byte_size(out, human_readable, limits.global_memory_cache_size, units);
        out = HMPC_FMTLIB::format_to(out, ", global memory cache line size: ");
        out = hmpc::fmt::format_maybe_human_readable_byte_size(out, human_readable, limits.global_memory_cache_line_size, units);
        out = HMPC_FMTLIB::format_to(out, ", local memory size: ");
        out = hmpc::fmt::format_maybe_human_readable_byte_size(out, human_readable, limits.local_memory_size, units);

        if (not no_braces)
        {
            out = HMPC_FMTLIB::format_to(out, "}}");
        }
        return out;
    }
};

/// Format hmpc::comp::device_info
///
/// The format specifier is
/// ```
/// f(optional) n(optional) (:[device_limits format specifier])?
/// ```
/// where
/// - 'f: flattens the formatting of the device_limits
/// - 'n': omits outer braces
template<typename Char>
struct HMPC_FMTLIB::formatter<hmpc::comp::device_info, Char>
{
    static constexpr bool is_specialized = true;

    bool no_braces = false;
    bool flatten = false;
    HMPC_FMTLIB::formatter<hmpc::comp::device_limits, Char> inner;

    constexpr auto parse(auto& ctx)
    {
        auto it = ctx.begin();
        if (it == ctx.end())
        {
            return it;
        }

        if (*it == Char{'f'})
        {
            flatten = true;
            ++it;
        }
        if (*it == Char{'n'})
        {
            no_braces = true;
            ++it;
        }
        if (it != ctx.end() and *it == Char{':'})
        {
            ctx.advance_to(++it);
            it = inner.parse(ctx);
        }
        if (it != ctx.end() and *it != Char{'}'})
        {
            throw HMPC_FMTLIB::format_error("Invalid format specifier");
        }

        return it;
    }

    std::string_view format(hmpc::comp::device_type type) const
    {
        using namespace std::literals::string_view_literals;

        switch (type)
        {
            case hmpc::comp::device_type::cpu:
                return "CPU"sv;
            case hmpc::comp::device_type::gpu:
                return "GPU"sv;
            default:
            case hmpc::comp::device_type::unknown:
                return "unknown"sv;
        }
    }

    auto format(hmpc::comp::device_info const& info, auto& ctx) const
    {
        auto out = ctx.out();
        if (not no_braces)
        {
            out = HMPC_FMTLIB::format_to(out, "{{");
        }

        out = HMPC_FMTLIB::format_to(out, "type: {}, name: {}, vendor: {}, driver: {}, available: {}, address bits: {}, address align bits: {}, ", format(info.type), info.name, info.vendor, info.driver, info.available, info.address_bits, info.address_align_bits);
        if (not flatten)
        {
            out = HMPC_FMTLIB::format_to(out, "limits: ");
        }

        ctx.advance_to(out);
        out = inner.format(info.limits, ctx);

        if (not no_braces)
        {
            out = HMPC_FMTLIB::format_to(out, "}}");
        }
        return out;
    }
};
