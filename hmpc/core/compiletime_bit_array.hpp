#pragma once

#include <hmpc/core/compiletime_bit_span.hpp>

namespace hmpc::core
{
    template<typename Limb, typename Normalization = hmpc::access::unnormal_tag>
    struct compiletime_bit_array
    {
        using limb_type = Limb;
        using normal_type = Normalization;

        static constexpr hmpc::signedness signedness = hmpc::without_sign;
        static constexpr hmpc::size limb_bit_size = limb_type::bit_size;

        hmpc::size bit_size;
        hmpc::size limb_size;
        limb_type* data;

        template<typename... Args>
        consteval compiletime_bit_array(hmpc::size bit_size, Args&&... args)
            : bit_size(bit_size)
            , limb_size(limb_size_for<limb_type>(bit_size))
        {
            if (limb_size > 0)
            {
                data = new limb_type[limb_size]{std::forward<Args>(args)...};
            }
            else
            {
                data = nullptr;
            }
        }

        constexpr ~compiletime_bit_array()
        {
            if (data != nullptr)
            {
                delete[] data;
            }
        }

        template<typename Self, hmpc::access::can_access<Self> Access = hmpc::access::traits::access_like_t<Self>>
        consteval auto compiletime_span(this Self&& self, Access = {})
        {
            return compiletime_bit_span<limb_type, Access, normal_type>{self.bit_size, self.data};
        }
    };
}
