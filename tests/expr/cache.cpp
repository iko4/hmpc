#include "catch_helpers.hpp"

#include <hmpc/comp/tensor.hpp>
#include <hmpc/expr/binary_expression.hpp>
#include <hmpc/expr/tensor.hpp>
#include <hmpc/expr/cache.hpp>
#include <hmpc/expr/number_theoretic_transform.hpp>
#include <hmpc/expr/random/binomial.hpp>
#include <hmpc/ints/poly_mod.hpp>
#include <hmpc/ints/uint.hpp>

TEST_CASE("Cache", "[expr][cache]")
{
    SECTION("Basic addition")
    {
        using uint = hmpc::ints::uint<64>;

        constexpr hmpc::size N = 10;

        auto x = hmpc::comp::make_tensor<uint>(hmpc::shape{N});

        using namespace hmpc::expr::operators;

        auto x_expr = hmpc::expr::tensor(x);
        auto expr = x_expr + hmpc::expr::cache(x_expr);
        auto cached_expr = hmpc::expr::cache(expr);
        auto traces = hmpc::expr::detail::trace_expressions(hmpc::detail::type_map{}, cached_expr, cached_expr);
        REQUIRE(traces.size == 1);
        REQUIRE(traces.get(hmpc::detail::tag_of<decltype(hmpc::expr::cache(x_expr))>).size == 1);

        auto&& cache = hmpc::expr::generate_execution_cache(expr, expr);
        REQUIRE(cache.size == 1);
    }

    SECTION("RNG + NTT")
    {
        constexpr auto p = hmpc::ints::ubigint<5>{17};
        constexpr auto N = 8;
        using R = hmpc::ints::poly_mod<p, N, hmpc::ints::coefficient_representation>;

        auto shape = hmpc::shape{2};

        using namespace hmpc::expr::operators;

        auto u = hmpc::expr::number_theoretic_transform(hmpc::expr::random::centered_binomial<R>(shape, hmpc::constants::half));
        auto v = hmpc::expr::number_theoretic_transform(hmpc::expr::random::centered_binomial<R>(shape, hmpc::constants::ten));
        auto w = hmpc::expr::number_theoretic_transform(hmpc::expr::random::centered_binomial<R>(shape, hmpc::constants::ten));

        REQUIRE(u.shape().size() == 2);
        REQUIRE(u.inner.shape().size() == 2);

        auto&& cache = hmpc::expr::generate_execution_cache(u + v, w);
        REQUIRE(cache.size == 4);
        REQUIRE(cache.get(hmpc::constants::zero).shape().size() == 2);
        REQUIRE(cache.get(hmpc::constants::one).shape().size() == 2);
        REQUIRE(cache.get(hmpc::constants::two).shape().size() == 2);
    }
}
