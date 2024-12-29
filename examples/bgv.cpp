#include <hmpc/comp/accessor.hpp>
#include <hmpc/comp/tensor.hpp>
#include <hmpc/comp/queue.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/expr/constant.hpp>
#include <hmpc/expr/number_theoretic_transform.hpp>
#include <hmpc/expr/random/binomial.hpp>
#include <hmpc/expr/unsqueeze.hpp>
#include <hmpc/ints/poly_mod.hpp>
#include <hmpc/ints/literals.hpp>
#include <hmpc/random/binomial.hpp>
#include <hmpc/random/number_generator.hpp>

#include <fmt/format.h>
#include <sycl/sycl.hpp>

#include <charconv>
#include <chrono>

auto main(int argc, char** raw_argv) -> int
{
    using namespace hmpc::ints::literals;
    using namespace hmpc::expr::operators;
    constexpr auto q = 0x2faeadbe7a0195c011ac195ad10269830e8001_int; // 1'063'351'684'119'354'455'646'439'919'975'158'459'206'107'137
    constexpr hmpc::size N = 1 << 14;
    using R = hmpc::ints::poly_mod<q, N, hmpc::ints::coefficient_representation>;
    using mod_q = R::element_type;
    constexpr auto p = hmpc::expr::constant_of<mod_q{0x89d046e0ae640001_int}>; // 9'930'515'109'164'351'489

    std::size_t n = 100;
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

    auto a_tensor = hmpc::comp::make_tensor<R>(hmpc::shape{});
    auto b_tensor = hmpc::comp::make_tensor<R>(hmpc::shape{});
    auto m_tensor = hmpc::comp::make_tensor<R>(hmpc::shape{n});

    sycl_queue.submit([&](auto& handler)
    {
        hmpc::comp::device_accessor a(a_tensor, handler, hmpc::access::discard_write);
        hmpc::comp::device_accessor b(b_tensor, handler, hmpc::access::discard_write);

        handler.parallel_for(sycl::range{N}, [=](hmpc::size i)
        {
            a[i] = static_cast<mod_q>(hmpc::ints::ubigint<32>{argc + n + i});

            b[i] = static_cast<mod_q>(hmpc::ints::ubigint<32>{argc + N - i});
        });
    });
    sycl_queue.submit([&](auto& handler)
    {
        hmpc::comp::device_accessor m(m_tensor, handler, hmpc::access::discard_write);

        handler.parallel_for(sycl::range{N * n}, [=](hmpc::size i)
        {
            m[i] = static_cast<mod_q>(hmpc::ints::ubigint<32>{argc + n - i});
        });
    });
    auto [ntt_a, ntt_b, ntt_m] = queue(
        hmpc::expr::number_theoretic_transform(hmpc::expr::tensor(a_tensor)),
        hmpc::expr::number_theoretic_transform(hmpc::expr::tensor(b_tensor)),
        hmpc::expr::number_theoretic_transform(hmpc::expr::tensor(m_tensor))
    );

    queue.wait();
    auto start = std::chrono::high_resolution_clock::now();

    auto a = hmpc::expr::tensor(ntt_a);
    auto b = hmpc::expr::tensor(ntt_b);
    auto m = hmpc::expr::tensor(ntt_m);
    auto u = hmpc::expr::number_theoretic_transform(hmpc::expr::random::centered_binomial<R>(hmpc::shape{n}, hmpc::constants::half));
    auto v = hmpc::expr::number_theoretic_transform(hmpc::expr::random::centered_binomial<R>(hmpc::shape{n}, hmpc::constants::ten));
    auto w = hmpc::expr::number_theoretic_transform(hmpc::expr::random::centered_binomial<R>(hmpc::shape{n}, hmpc::constants::ten));
    static_assert(not std::same_as<decltype(v), decltype(w)>);

    auto [c0, c1] = queue(
        hmpc::expr::unsqueeze(b, hmpc::constants::minus_one) * u + v * p + m,
        hmpc::expr::unsqueeze(a, hmpc::constants::minus_one) * u + w * p
    );

    queue.wait();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    fmt::print("{} {:2.10f}\n", N * n, duration.count());
}
