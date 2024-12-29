#pragma once

#include <type_traits>

namespace hmpc::access
{
    struct noaccess_tag
    {
    };

    struct read_tag
    {
    };
    struct read_write_tag
    {
    };
    struct discard_write_tag
    {
    };
    struct discard_read_write_tag
    {
    };

    constexpr noaccess_tag noaccess = {};
    constexpr read_write_tag read_write = {};

    struct write_tag
    {
        friend constexpr read_write_tag operator|(read_tag, write_tag) noexcept
        {
            return read_write;
        }
    };

    constexpr read_tag read = {};
    constexpr write_tag write = {};
    constexpr discard_write_tag discard_write = {};
    constexpr discard_read_write_tag discard_read_write = {};

    struct discard_tag
    {
        friend constexpr discard_write_tag operator|(discard_tag, write_tag) noexcept
        {
            return discard_write;
        }

        friend constexpr discard_read_write_tag operator|(discard_tag, read_write_tag) noexcept
        {
            return discard_read_write;
        }
    };

    constexpr discard_tag discard = {};

    namespace traits
    {
        template<typename Access>
        struct is_read : std::false_type
        {
        };
        template<>
        struct is_read<read_tag> : std::true_type
        {
        };
        template<>
        struct is_read<read_write_tag> : std::true_type
        {
        };
        template<>
        struct is_read<discard_read_write_tag> : std::true_type
        {
        };
        template<typename Access>
        constexpr bool is_read_v = is_read<Access>::value;

        template<typename Access>
        struct is_write : std::false_type
        {
        };
        template<>
        struct is_write<write_tag> : std::true_type
        {
        };
        template<>
        struct is_write<read_write_tag> : std::true_type
        {
        };
        template<>
        struct is_write<discard_write_tag> : std::true_type
        {
        };
        template<>
        struct is_write<discard_read_write_tag> : std::true_type
        {
        };
        template<typename Access>
        constexpr bool is_write_v = is_write<Access>::value;

        template<typename Access>
        struct is_discard : std::false_type
        {
        };
        template<>
        struct is_discard<discard_write_tag> : std::true_type
        {
        };
        template<>
        struct is_discard<discard_read_write_tag> : std::true_type
        {
        };
        template<typename Access>
        constexpr bool is_discard_v = is_discard<Access>::value;
    }

    template<typename Access>
    concept is_read = traits::is_read_v<Access>;
    template<typename Access>
    concept is_write = traits::is_write_v<Access>;
    template<typename Access>
    concept is_read_only = is_read<Access> and not is_write<Access>;
    template<typename Access>
    concept is_write_only = is_write<Access> and not is_read<Access>;
    template<typename Access>
    concept is_read_write = is_read<Access> and is_write<Access>;
    template<typename Access>
    concept is_access = is_read<Access> or is_write<Access>;
    template<typename Access>
    concept is_discard = traits::is_discard_v<Access>;

    struct normal_tag
    {
    };
    constexpr normal_tag normal = {};
    struct unnormal_tag
    {
    };
    constexpr unnormal_tag unnormal = {};

    namespace traits
    {
        template<typename Normalization>
        struct is_normal : std::false_type
        {
        };
        template<>
        struct is_normal<normal_tag> : std::true_type
        {
        };
        template<typename Normalization>
        constexpr bool is_normal_v = is_normal<Normalization>::value;

        template<typename Normalization, typename OtherNormalization>
        struct common_normal
        {
            using type = unnormal_tag;
        };
        template<>
        struct common_normal<normal_tag, normal_tag>
        {
            using type = normal_tag;
        };
        template<typename Normalization, typename OtherNormalization>
        using common_normal_t = common_normal<Normalization, OtherNormalization>::type;
    }

    template<typename Normalization>
    concept is_normal = traits::is_normal_v<Normalization>;
    template<typename Normalization>
    concept is_unnormal = not is_normal<Normalization>;

    namespace traits
    {
        template<typename T, typename Access>
        struct access_as;
        template<typename T, hmpc::access::is_read_only Access>
        struct access_as<T, Access>
        {
            using type = T const;
        };
        template<typename T, hmpc::access::is_write Access>
        struct access_as<T, Access>
        {
            using type = T;
        };
        template<typename T, typename Access>
        using access_as_t = access_as<T, Access>::type;
        template<typename T, typename Access>
        using pointer_t = access_as_t<T, Access>*;
        template<typename T, typename Access>
        using reference_t = access_as_t<T, Access>&;

        template<typename T>
        struct access_like;
        template<typename T>
        struct access_like<T const&>
        {
            using type = read_tag;
        };
        template<typename T>
        struct access_like<T&>
        {
            using type = read_write_tag;
        };
        template<typename T>
        using access_like_t = access_like<T>::type;

        /// ### Note
        /// The result will be formed while ignoring `hmpc::access::is_discard` for both types.
        /// In particular, the result will always be a type without a discard qualifier.
        template<typename Access, typename OtherAccess>
        struct common_access
        {
            using type = noaccess_tag;
        };
        template<typename Access, typename OtherAccess>
            requires (hmpc::access::is_read<Access> and hmpc::access::is_read<OtherAccess> and (hmpc::access::is_read_only<Access> or hmpc::access::is_read_only<OtherAccess>))
        struct common_access<Access, OtherAccess>
        {
            using type = read_tag;
        };
        template<typename Access, typename OtherAccess>
            requires (hmpc::access::is_write<Access> and hmpc::access::is_write<OtherAccess> and (hmpc::access::is_write_only<Access> or hmpc::access::is_write_only<OtherAccess>))
        struct common_access<Access, OtherAccess>
        {
            using type = write_tag;
        };
        template<typename Access, typename OtherAccess>
            requires (hmpc::access::is_read_write<Access> and hmpc::access::is_read_write<OtherAccess>)
        struct common_access<Access, OtherAccess>
        {
            using type = read_write_tag;
        };
        template<typename Access, typename OtherAccess>
        using common_access_t = common_access<Access, OtherAccess>::type;
    }

    template<typename Access, typename T>
    concept can_access = is_access<traits::common_access_t<Access, traits::access_like_t<T>>>;

    struct once_tag
    {
    };
    constexpr once_tag once = {};
    struct multiple_tag
    {
    };
    constexpr multiple_tag multiple = {};

    enum class pattern
    {
        none,
        once,
        multiple,
    };

    namespace traits
    {
        template<typename Pattern>
        struct access_pattern
        {
        };
        template<>
        struct access_pattern<once_tag> : std::integral_constant<pattern, pattern::once>
        {
        };
        template<>
        struct access_pattern<multiple_tag> : std::integral_constant<pattern, pattern::multiple>
        {
        };
        template<pattern Pattern>
        struct access_pattern<std::integral_constant<pattern, Pattern>> : std::integral_constant<pattern, Pattern>
        {
        };
        template<typename Pattern>
        constexpr auto access_pattern_v = access_pattern<Pattern>::value;
    }
}
