#pragma once

namespace hmpc::detail
{
    template<auto Tag = []{}, typename T, typename... Args>
    consteval auto unique_tag(T, Args&&...) noexcept
    {
        return Tag;
    }
}
