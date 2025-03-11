#include "catch_helpers.hpp"

#include <hmpc/ints/bigint.hpp>
#include <hmpc/net/queue.hpp>

#include <optional>
#include <thread>
#include <vector>

constexpr auto config = "tests/mpc.yaml";

TEST_CASE("Network: Broadcast", "[net][queue][ffi][broadcast]")
{
    static constexpr auto communicator = hmpc::net::communicator_for<0, 1, 2, 3>;

    using uint = hmpc::ints::ubigint<96, hmpc::core::uint32>;
    static_assert(uint::limb_size == 3);

    static_assert(communicator.size == 4);

    auto queues = std::make_tuple(
        hmpc::net::queue(hmpc::party_constant_of<0>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<1>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<2>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<3>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<4>, hmpc::net::config::read_env(config)) // not part of the communicator
    );

    auto shape = hmpc::shape{2};

    auto data = std::array
    {
        hmpc::comp::make_tensor<uint>(shape),
        hmpc::comp::make_tensor<uint>(shape),
        hmpc::comp::make_tensor<uint>(shape),
        hmpc::comp::make_tensor<uint>(shape),
        hmpc::comp::make_tensor<uint>(shape),
    };

    // Party 4 sends data
    {
        hmpc::comp::host_accessor access(data[4], hmpc::access::discard_write);
        access[0] = uint{4, 1, 2};
        access[1] = uint{4, 3, 4};
    }

    // scope for threads
    {
        std::vector<std::jthread> threads;
        threads.reserve(communicator.size + 1);

        hmpc::iter::for_range<communicator.size + 1>([&](auto i)
        {
            threads.emplace_back(
                std::jthread([&, i]()
                {
                    if constexpr (i < communicator.size)
                    {
                        data[i] = std::get<i>(queues).template broadcast<uint>(communicator, hmpc::party_constant_of<4>, shape);
                    }
                    else
                    {
                        std::get<i>(queues).broadcast(communicator, hmpc::party_constant_of<4>, data[i]);
                    }
                })
            );
        });
    }

    for (hmpc::size i = 0; i < communicator.size + 1; ++i)
    {
        hmpc::comp::host_accessor access(data[i], hmpc::access::read);
        CHECK(access[0] == uint{4, 1, 2});
        CHECK(access[1] == uint{4, 3, 4});
    }

#ifdef HMPC_ENABLE_STATISTICS
    CHECK(std::get<0>(queues).stats() == hmpc::net::statistics{.sent = 0, .received = 24, .rounds = 1});
    CHECK(std::get<1>(queues).stats() == hmpc::net::statistics{.sent = 0, .received = 24, .rounds = 1});
    CHECK(std::get<2>(queues).stats() == hmpc::net::statistics{.sent = 0, .received = 24, .rounds = 1});
    CHECK(std::get<3>(queues).stats() == hmpc::net::statistics{.sent = 0, .received = 24, .rounds = 1});
    CHECK(std::get<4>(queues).stats() == hmpc::net::statistics{.sent = 96, .received = 0, .rounds = 1});
#endif
}

TEST_CASE("Network: Gather", "[net][queue][ffi][gather]")
{
    static constexpr auto communicator = hmpc::net::communicator_for<10, 11, 12, 13>;

    using uint = hmpc::ints::ubigint<96, hmpc::core::uint32>;
    static_assert(uint::limb_size == 3);

    static_assert(communicator.size == 4);

    auto queues = std::make_tuple(
        hmpc::net::queue(hmpc::party_constant_of<10>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<11>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<12>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<13>, hmpc::net::config::read_env(config))
    );

    auto data = std::array
    {
        hmpc::comp::make_tensor<uint>(hmpc::shape{2}),
        hmpc::comp::make_tensor<uint>(hmpc::shape{2}),
        hmpc::comp::make_tensor<uint>(hmpc::shape{2}),
        hmpc::comp::make_tensor<uint>(hmpc::shape{2}),
    };
    using tensor_type = decltype(data)::value_type;

    for (hmpc::size i = 0; i < communicator.size; ++i)
    {
        hmpc::comp::host_accessor access(data[i], hmpc::access::discard_write);
        access[0] = uint{static_cast<hmpc::core::uint32>(i), 1, 2};
        access[1] = uint{static_cast<hmpc::core::uint32>(i), 3, 4};
    }

    std::optional<std::array<tensor_type, communicator.size>> result;

    // scope for threads
    {
        std::vector<std::jthread> threads;
        threads.reserve(communicator.size);

        hmpc::iter::for_range<communicator.size - 1>([&](auto i)
        {
            threads.emplace_back(
                std::jthread([&, i]()
                {
                    std::get<i>(queues).gather(communicator, hmpc::party_constant_of<13>, data[i]);
                })
            );
        });

        std::jthread([&]()
        {
            result = std::get<3>(queues).gather(communicator, hmpc::party_constant_of<13>, std::move(data[3]));
        });
    }

    REQUIRE(result.has_value());
    for (hmpc::size i = 0; i < communicator.size; ++i)
    {
        hmpc::comp::host_accessor access(result.value()[i], hmpc::access::read);
        CHECK(access[0] == uint{static_cast<hmpc::core::uint32>(i), 1, 2});
        CHECK(access[1] == uint{static_cast<hmpc::core::uint32>(i), 3, 4});
    }

#ifdef HMPC_ENABLE_STATISTICS
    CHECK(std::get<0>(queues).stats() == hmpc::net::statistics{.sent = 24, .received = 0, .rounds = 1});
    CHECK(std::get<1>(queues).stats() == hmpc::net::statistics{.sent = 24, .received = 0, .rounds = 1});
    CHECK(std::get<2>(queues).stats() == hmpc::net::statistics{.sent = 24, .received = 0, .rounds = 1});
    CHECK(std::get<3>(queues).stats() == hmpc::net::statistics{.sent = 0, .received = 72, .rounds = 1});
#endif
}

TEST_CASE("Network: All-gather", "[net][queue][ffi][all_gather]")
{
    static constexpr auto communicator = hmpc::net::communicator_for<20, 21, 22, 23>;

    using uint = hmpc::ints::ubigint<96, hmpc::core::uint32>;
    static_assert(uint::limb_size == 3);

    static_assert(communicator.size == 4);

    auto queues = std::make_tuple(
        hmpc::net::queue(hmpc::party_constant_of<20>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<21>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<22>, hmpc::net::config::read_env(config)),
        hmpc::net::queue(hmpc::party_constant_of<23>, hmpc::net::config::read_env(config))
    );

    auto data = std::array
    {
        hmpc::comp::make_tensor<uint>(hmpc::shape{2}),
        hmpc::comp::make_tensor<uint>(hmpc::shape{2}),
        hmpc::comp::make_tensor<uint>(hmpc::shape{2}),
        hmpc::comp::make_tensor<uint>(hmpc::shape{2}),
    };
    using tensor_type = decltype(data)::value_type;

    for (hmpc::size i = 0; i < communicator.size; ++i)
    {
        hmpc::comp::host_accessor access(data[i], hmpc::access::discard_write);
        access[0] = uint{static_cast<hmpc::core::uint32>(i), 1, 2};
        access[1] = uint{static_cast<hmpc::core::uint32>(i), 3, 4};
    }

    std::array<std::optional<std::array<tensor_type, communicator.size>>, communicator.size> results = {};

    // scope for threads
    {
        std::vector<std::jthread> threads;
        threads.reserve(communicator.size);

        hmpc::iter::for_range<communicator.size>([&](auto i)
        {
            threads.emplace_back(
                std::jthread([&, i]()
                {
                    results[i] = std::get<i>(queues).all_gather(communicator, std::move(data[i]));
                })
            );
        });
    }

    for (hmpc::size i = 0; i < communicator.size; ++i)
    {
        REQUIRE(results[i].has_value());
        for (hmpc::size j = 0; j < communicator.size; ++j)
        {
            hmpc::comp::host_accessor access(results[i].value()[j], hmpc::access::read);
            CHECK(access[0] == uint{static_cast<hmpc::core::uint32>(j), 1, 2});
            CHECK(access[1] == uint{static_cast<hmpc::core::uint32>(j), 3, 4});
        }
    }

#ifdef HMPC_ENABLE_STATISTICS
    CHECK(std::get<0>(queues).stats() == hmpc::net::statistics{.sent = 72, .received = 72, .rounds = 1});
    CHECK(std::get<1>(queues).stats() == hmpc::net::statistics{.sent = 72, .received = 72, .rounds = 1});
    CHECK(std::get<2>(queues).stats() == hmpc::net::statistics{.sent = 72, .received = 72, .rounds = 1});
    CHECK(std::get<3>(queues).stats() == hmpc::net::statistics{.sent = 72, .received = 72, .rounds = 1});
#endif
}

TEST_CASE("Network: All-to-all", "[net][queue][ffi][all_to_all]")
{
    // TODO
}
