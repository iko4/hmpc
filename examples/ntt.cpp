#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/queue.hpp>
#include <hmpc/comp/tensor.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/number_theoretic_transform.hpp>
#include <hmpc/expr/random/binomial.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/expr/unsqueeze.hpp>
#include <hmpc/ints/literals.hpp>
#include <hmpc/ints/poly_mod.hpp>

#include <sycl/sycl.hpp>

#include <charconv>
#include <chrono>
#include <print>

auto main(int argc, char** raw_argv) -> int
{
    using namespace hmpc::ints::literals;
    using namespace hmpc::expr::operators;
    constexpr auto p = 0x18f25cd9a75ccbd9c146d4abaec00001_int; // 33'159'890'964'594'391'257'671'722'130'143'707'137;
    constexpr hmpc::size N = 1 << 20;
    using R = hmpc::ints::poly_mod<p, N, hmpc::ints::coefficient_representation>;
    using mod_p = R::element_type;

    std::size_t n = 10;
    std::vector<std::string_view> argv(raw_argv, raw_argv + argc);
    if (argc > 1)
    {
        std::from_chars(argv[1].data(), argv[1].data() + argv[1].size(), n);
    }
    int processors = 1;
    if (argc > 2)
    {
        std::from_chars(argv[2].data(), argv[2].data() + argv[2].size(), processors);
    }

    sycl::queue sycl_queue(processors < 0 ? sycl::gpu_selector_v : sycl::cpu_selector_v);
    hmpc::comp::queue queue(sycl_queue);

    auto tensor = hmpc::comp::make_tensor<R>(hmpc::shape{n});

    sycl_queue.submit([&](auto& handler)
    {
        hmpc::comp::device_accessor x(tensor, handler, hmpc::access::discard_write);

        handler.parallel_for(sycl::range{N * n}, [=](hmpc::size i)
        {
            x[i] = static_cast<mod_p>(hmpc::ints::ubigint<32>{argc + N * n - i});
        });
    });

    sycl_queue.wait();
    auto start = std::chrono::high_resolution_clock::now();

    auto y = queue(hmpc::expr::number_theoretic_transform(hmpc::expr::tensor(tensor)));

    queue.wait();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::println("{} {:2.10f}", n, duration.count());
}
