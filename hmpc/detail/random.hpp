#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <random>

namespace hmpc::detail
{
    template<typename T>
    void fill_random(T& value)
    {
        // Implementation of std::random_device seems to be reasonable on mayor platforms
        // - https://releases.llvm.org/15.0.0/projects/libcxx/docs/ReleaseNotes.html#abi-affecting-changes
        // - https://learn.microsoft.com/en-us/cpp/standard-library/random-device-class
        // - https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html
        std::random_device random_device;

        using block_type = std::random_device::result_type;

        static_assert(sizeof(T) % sizeof(block_type) == 0);

        constexpr std::size_t block_count = sizeof(T) / sizeof(block_type);

        std::array<block_type, block_count> buffer;
        std::ranges::generate(buffer, [&](){ return random_device(); });

        value = std::bit_cast<T>(buffer);
    }
}
