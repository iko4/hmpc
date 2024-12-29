#include <fmt/format.h>

#include "FHE/DiscreteGauss.h"
#include "FHE/Ring_Element.h"
#include <charconv>
#include <chrono>

auto main(int argc, char** raw_argv) -> int
{
    auto const q = bigint("1063351684119354455646439919975158459206107137");
    constexpr auto N = 1 << 14;
    auto const ring = Ring(2 * N);
    auto const z = Zp_Data(q, true);
    auto const fft = FFT_Data(ring, z);
    modp p;
    to_modp(p, bigint("9930515109164351489"), z);

    using R = Ring_Element;
    using mod_q = modp;

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

    auto a = Ring_Element(fft);
    auto b = Ring_Element(fft);
    auto m = std::vector<Ring_Element>(n, Ring_Element(fft));

    a.allocate();
    b.allocate();
    #pragma omp parallel for
    for (int i = 0; i < N; ++i)
    {
        bigint x = argc + n + i;
        bigint y = argc + N - i;

        modp xx;
        modp yy;

        to_modp(xx, x, z);
        to_modp(yy, y, z);
        a.set_element(i, xx);
        b.set_element(i, yy);
    }
    a.change_rep(evaluation);
    b.change_rep(evaluation);

    #pragma omp parallel for
    for (int j = 0; j < n; ++j)
    {
        m[j].allocate();
        #pragma omp parallel for
        for (int i = 0; i < N; ++i)
        {
            bigint x = argc + n - (i + N * j);

            modp xx;

            to_modp(xx, x, z);
            m[j].set_element(i, xx);
        }
        m[j].change_rep(evaluation);
    }

    auto start = std::chrono::high_resolution_clock::now();

    #pragma omp parallel for
    for (int j = 0; j < n; ++j)
    {
        PRNG rng;
        rng.ReSeed();
        DiscreteGauss g(sqrt(10));

        auto u = Ring_Element(fft);
        auto v = Ring_Element(fft);
        auto w = Ring_Element(fft);

        for (int i = 0; i < N; ++i)
        {
            bigint uu = sample_half(rng);
            bigint vv = g.sample(rng);
            bigint ww = g.sample(rng);

            modp uuu;
            modp vvv;
            modp www;

            to_modp(uuu, uu, z);
            to_modp(vvv, vv, z);
            to_modp(www, ww, z);

            u.set_element(i, uuu);
            v.set_element(i, vvv);
            w.set_element(i, www);
        }
        u.change_rep(evaluation);
        v.change_rep(evaluation);
        w.change_rep(evaluation);

        auto c0 = b;
        c0 *= u;
        c0 += (v *= p);
        c0 += m[j];
        auto c1 = a;
        c1 *= (u *= p);
        c1 += w;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    fmt::print("{} {:2.10f}\n", N * n, duration.count());
}
