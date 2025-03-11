#include "FHE/Ring_Element.h"

#include <charconv>
#include <chrono>
#include <print>

auto main(int argc, char** raw_argv) -> int
{
    auto const p = bigint("33159890964594391257671722130143707137");
    constexpr auto N = 1 << 20;
    auto const ring = Ring(2 * N);
    auto const z = Zp_Data(p, true);
    auto const fft = FFT_Data(ring, z);

    using R = Ring_Element;
    using mod_p = modp;

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

    auto x = std::vector<Ring_Element>(n, Ring_Element(fft));

    #pragma omp parallel for
    for (int j = 0; j < n; ++j)
    {
        x[j].allocate();
        #pragma omp parallel for
        for (int i = 0; i < N; ++i)
        {
            bigint value = argc + N * n - (i + N * j);

            modp v;

            to_modp(v, value, z);
            x[j].set_element(i, v);
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    #pragma omp parallel for
    for (int j = 0; j < n; ++j)
    {
        x[j].change_rep(evaluation);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::println("{} {:2.10f}", n, duration.count());
}
