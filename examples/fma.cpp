#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/tensor.hpp>
#include <hmpc/ints/literals.hpp>
#include <hmpc/ints/mod.hpp>

#include <sycl/sycl.hpp>

#include <charconv>
#include <chrono>
#include <print>

auto main(int argc, char** raw_argv) -> int
{
    using namespace hmpc::ints::literals;
    constexpr auto p = 0x95d13129b10a9d6e4bfc74319391cce9_int; // 199'141'158'556'603'165'496'448'001'566'829'563'113

    using mod_p = hmpc::ints::mod<p>;

    std::size_t N = 10'000'000;
    std::vector<std::string_view> argv(raw_argv, raw_argv + argc);
    if (argc > 1)
    {
        std::from_chars(argv[1].data(), argv[1].data() + argv[1].size(), N);
    }
    int processors = 1;
    if (argc > 2)
    {
        std::from_chars(argv[2].data(), argv[2].data() + argv[2].size(), processors);
    }

    auto a_tensor = hmpc::comp::make_tensor<mod_p>(hmpc::shape{N});
    auto b_tensor = hmpc::comp::make_tensor<mod_p>(hmpc::shape{N});
    auto c_tensor = hmpc::comp::make_tensor<mod_p>(hmpc::shape{N});
    auto x_tensor = hmpc::comp::make_tensor<mod_p>(hmpc::shape{N});

    sycl::queue queue(processors < 0 ? sycl::gpu_selector_v : sycl::cpu_selector_v);

    queue.submit([&](auto& handler)
    {
        hmpc::comp::device_accessor a(a_tensor, handler, hmpc::access::discard_write);
        hmpc::comp::device_accessor b(b_tensor, handler, hmpc::access::discard_write);
        hmpc::comp::device_accessor c(c_tensor, handler, hmpc::access::discard_write);

        handler.parallel_for(sycl::range{N}, [=](hmpc::size i)
        {
            a[i] = static_cast<mod_p>(hmpc::ints::ubigint<32>{argc + i});

            b[i] = static_cast<mod_p>(hmpc::ints::ubigint<32>{argc + N - i});

            c[i] = static_cast<mod_p>(hmpc::ints::ubigint<32>{argc + N + i});
        });
    });

    queue.wait();
    auto start = std::chrono::high_resolution_clock::now();
    queue.submit([&](auto& handler)
    {
        hmpc::comp::device_accessor a(a_tensor, handler, hmpc::access::read);
        hmpc::comp::device_accessor b(b_tensor, handler, hmpc::access::read);
        hmpc::comp::device_accessor c(c_tensor, handler, hmpc::access::read);
        hmpc::comp::device_accessor x(x_tensor, handler, hmpc::access::discard_write);

        handler.parallel_for(sycl::range{N}, [=](hmpc::size i)
        {
            x[i] = a[i] * b[i] + c[i];
        });
    });
    queue.wait();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::println("{} {:2.10f}", N, duration.count());
}
