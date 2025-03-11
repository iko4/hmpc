#pragma once

#include <hmpc/config.hpp>

#include <format>

namespace hmpc::fmt
{
    namespace units
    {
        enum class binary
        {
            none,
            bit,
            byte,
        };
    }

    template<typename T>
    constexpr auto format_byte_size(auto out, T value, units::binary u) noexcept
    {
        switch (u)
        {
        case units::binary::none:
            return std::format_to(out, "{}", value);
        case units::binary::bit:
            return std::format_to(out, "{} b", value * T{8});
        case units::binary::byte:
            return std::format_to(out, "{} B", value);
        }
    }

    template<typename T>
    constexpr auto format_human_readable_byte_size(auto out, T value, units::binary u) noexcept
    {
        if (u == units::binary::bit)
        {
            value = value * T{8};
        }

        if (value < 0x10'0000) // 1024 * 1024
        {
            switch (u)
            {
            case units::binary::none:
                return std::format_to(out, "{}", value);
            case units::binary::bit:
                return std::format_to(out, "{} b", value);
            case units::binary::byte:
                return std::format_to(out, "{} B", value);
            }
        }
        value /= 1024;
        if (value < 0x10'0000) // 1024 * 1024
        {
            switch (u)
            {
            case units::binary::none:
                return std::format_to(out, "{} Ki", value);
            case units::binary::bit:
                return std::format_to(out, "{} Kib", value);
            case units::binary::byte:
                return std::format_to(out, "{} KiB", value);
            }
        }
        value /= 1024;
        if (value < 0x10'0000) // 1024 * 1024
        {
            switch (u)
            {
            case units::binary::none:
                return std::format_to(out, "{} Mi", value);
            case units::binary::bit:
                return std::format_to(out, "{} Mib", value);
            case units::binary::byte:
                return std::format_to(out, "{} MiB", value);
            }
        }
        value /= 1024;
        if (value < 0x10'0000) // 1024 * 1024
        {
            switch (u)
            {
            case units::binary::none:
                return std::format_to(out, "{} Gi", value);
            case units::binary::bit:
                return std::format_to(out, "{} Gib", value);
            case units::binary::byte:
                return std::format_to(out, "{} GiB", value);
            }
        }
        value /= 1024;
        if (value < 0x10'0000) // 1024 * 1024
        {
            switch (u)
            {
            case units::binary::none:
                return std::format_to(out, "{} Ti", value);
            case units::binary::bit:
                return std::format_to(out, "{} Tib", value);
            case units::binary::byte:
                return std::format_to(out, "{} TiB", value);
            }
        }
        value /= 1024;
        switch (u)
        {
        case units::binary::none:
            return std::format_to(out, "{} Pi", value);
        case units::binary::bit:
            return std::format_to(out, "{} Pib", value);
        case units::binary::byte:
            return std::format_to(out, "{} PiB", value);
        }
    }

    template<typename T>
    constexpr auto format_maybe_human_readable_byte_size(auto out, bool human_readable, T value, units::binary u) noexcept
    {
        if (human_readable)
        {
            return format_human_readable_byte_size(out, value, u);
        }
        else
        {
            return format_byte_size(out, value, u);
        }
    }
}
