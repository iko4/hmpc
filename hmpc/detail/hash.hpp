#pragma once

#include <string_view>

namespace hmpc::detail
{
    /// Combining hashes as suggested in [1], which is based on [2], which is based on [3].
    /// #### Algorithm reference
    /// - [1] Nicolai Josuttis: "P0814R2: hash_combine() Again." Online, 2018. [Link](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0814r2.pdf), accessed 2024-03-19.
    /// - [2] Boost Contributors, Daniel James, Peter Dimov: "Boost.ContainerHash: hash_combine." Online, 2022. [Link](https://www.boost.org/doc/libs/1_84_0/libs/container_hash/doc/html/hash.html#notes_hash_combine), accessed 2024-03-19.
    /// - [3] Timothy C. Hoad, Justin Zobel: "Methods for Identifying Versioned and Plagiarized Documents." Journal of the American Society for Information Science and Technology, Volume 54, Numer 3, 2003. [Link](https://people.eng.unimelb.edu.au/jzobel/fulltext/jasist03thz.pdf), accessed 2024-03-19.
    constexpr std::size_t combine_hashes(std::size_t hash, std::size_t other_hash) noexcept
    {
        return hash xor (other_hash + 0x9e37'79b9u + (hash << 6) + (hash >> 2));
    }

    template<std::same_as<std::size_t>... Hashes>
    constexpr std::size_t combine_hashes(std::size_t hash, Hashes... hashes) noexcept
    {
        if constexpr (sizeof...(Hashes) == 0)
        {
            return hash;
        }
        else
        {
            return ((hash = combine_hashes(hash, hashes)), ...);
        }
    }
}
