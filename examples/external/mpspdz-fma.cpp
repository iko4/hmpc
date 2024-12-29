#include <fmt/format.h>

#include "Math/modp.hpp"
#include <charconv>
#include <chrono>

auto main(int argc, char** raw_argv) -> int
{
    auto const p = bigint("199141158556603165496448001566829563113");
    auto const z = Zp_Data(p, true);

    using mod_p = modp;

    std::size_t N = 10'000'000;
    std::vector<std::string_view> argv(raw_argv, raw_argv + argc);
    if (argc > 1)
    {
        std::from_chars(argv[1].data(), argv[1].data() + argv[1].size(), N);
    }

    std::vector<mod_p> a(N);
    std::vector<mod_p> b(N);
    std::vector<mod_p> c(N);
    std::vector<mod_p> x(N);
    #pragma omp parallel for
    for (std::size_t i = 0; i < N; ++i)
    {
        bigint u = argc + i;
        bigint v = argc + N - i;
        bigint w = argc + N + i;

        to_modp(a[i], u, z);
        to_modp(b[i], v, z);
        to_modp(c[i], w, z);
    }

    auto start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for
    for (std::size_t i = 0; i < N; ++i)
    {
        mod_p t;
        Mul(t, a[i], b[i], z);
        Add(x[i], t, c[i], z);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    fmt::print("{} {:2.10f}\n", N, duration.count());
}
