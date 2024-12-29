#include "catch_helpers.hpp"

#include <hmpc/index.hpp>
#include <hmpc/shape.hpp>
#include <hmpc/comp/vector.hpp>

TEST_CASE("Shape", "[shape]")
{
    SECTION("Null shape")
    {
        auto shape = hmpc::shape{};
        auto size = shape.size();
        REQUIRE(shape.rank == 0);
        REQUIRE(size == 1);
        REQUIRE(hmpc::is_constant_of<decltype(size), hmpc::size>);
    }

    SECTION("1D dynamic shape")
    {
        auto shape = hmpc::shape{42u};
        auto size = shape.size();
        auto dim0 = shape.get(hmpc::constants::zero);
        REQUIRE(shape.rank == 1);
        REQUIRE(size == 42);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == 42);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);

        auto [x] = shape;
        REQUIRE(x == 42);
    }

    SECTION("1D fixed shape")
    {
        auto shape = hmpc::shape<42>{};
        auto size = shape.size();
        auto dim0 = shape.get(hmpc::constants::zero);
        REQUIRE(shape.rank == 1);
        REQUIRE(size == 42);
        REQUIRE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == 42);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);

        auto [x] = shape;
        REQUIRE(x == 42);
    }

    SECTION("1D fixed shape")
    {
        auto shape = hmpc::shape{hmpc::size_constant_of<42>};
        auto size = shape.size();
        auto dim0 = shape.get(hmpc::constants::zero);
        REQUIRE(shape.rank == 1);
        REQUIRE(size == 42);
        REQUIRE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == 42);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
    }

    SECTION("1D placeholder shape")
    {
        auto shape = hmpc::shape{hmpc::constants::placeholder};
        auto size = shape.size();
        auto dim0 = shape.get(hmpc::constants::zero);
        REQUIRE(shape.rank == 1);
        REQUIRE(size == 1);
        REQUIRE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == hmpc::placeholder_extent);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
    }

    SECTION("2D dynamic shape")
    {
        auto shape = hmpc::shape{42, 11};
        auto size = shape.size();
        auto dim0 = shape.get<0>();
        auto dim1 = shape.get<1>();
        REQUIRE(shape.rank == 2);
        REQUIRE(size == 462);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == 42);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(dim1 == 11);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(dim1), hmpc::size>);

        auto [x, y] = shape;
        REQUIRE(x == 42);
        REQUIRE(y == 11);
    }

    SECTION("2D fixed shape")
    {
        auto shape = hmpc::shape<42, 11>{};
        auto size = shape.size();
        auto dim0 = shape.get<0>();
        auto dim1 = shape.get<1>();
        REQUIRE(shape.rank == 2);
        REQUIRE(size == 462);
        REQUIRE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == 42);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(dim1 == 11);
        REQUIRE(hmpc::is_constant_of<decltype(dim1), hmpc::size>);

        auto [x, y] = shape;
        REQUIRE(x == 42);
        REQUIRE(y == 11);
    }

    SECTION("2D mixed shape")
    {
        auto shape = hmpc::shape{hmpc::size_constant_of<42>, 11};
        auto size = shape.size();
        auto dim0 = shape.get<0>();
        auto dim1 = shape.get<1>();
        REQUIRE(shape.rank == 2);
        REQUIRE(size == 462);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == 42);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(dim1 == 11);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(dim1), hmpc::size>);

        auto [x, y] = shape;
        REQUIRE(x == 42);
        REQUIRE(y == 11);
    }

    SECTION("2D mixed shape")
    {
        auto shape = hmpc::shape{42, hmpc::size_constant_of<11>};
        auto size = shape.size();
        auto dim0 = shape.get<0>();
        auto dim1 = shape.get<1>();
        REQUIRE(shape.rank == 2);
        REQUIRE(size == 462);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == 42);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(dim1 == 11);
        REQUIRE(hmpc::is_constant_of<decltype(dim1), hmpc::size>);

        auto [x, y] = shape;
        REQUIRE(x == 42);
        REQUIRE(y == 11);
    }

    SECTION("2D placeholder shape")
    {
        auto shape = hmpc::shape{42, hmpc::constants::placeholder};
        auto size = shape.size();
        auto dim0 = shape.get<0>();
        auto dim1 = shape.get<1>();
        REQUIRE(shape.rank == 2);
        REQUIRE(size == 42);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == 42);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(dim1 == hmpc::placeholder_extent);
        REQUIRE(hmpc::is_constant_of<decltype(dim1), hmpc::size>);

        auto [x, y] = shape;
        REQUIRE(x == 42);
        REQUIRE(y == hmpc::placeholder_extent);
    }

    SECTION("2D placeholder shape")
    {
        auto shape = hmpc::shape{hmpc::size_constant_of<42>, hmpc::constants::placeholder};
        auto size = shape.size();
        auto dim0 = shape.get<0>();
        auto dim1 = shape.get<1>();
        REQUIRE(shape.rank == 2);
        REQUIRE(size == 42);
        REQUIRE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == 42);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(dim1 == hmpc::placeholder_extent);
        REQUIRE(hmpc::is_constant_of<decltype(dim1), hmpc::size>);

        auto [x, y] = shape;
        REQUIRE(x == 42);
        REQUIRE(y == hmpc::placeholder_extent);
    }

    SECTION("2D placeholder shape")
    {
        auto shape = hmpc::shape{hmpc::constants::placeholder, 11};
        auto size = shape.size();
        auto dim0 = shape.get<0>();
        auto dim1 = shape.get<1>();
        REQUIRE(shape.rank == 2);
        REQUIRE(size == 11);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == hmpc::placeholder_extent);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(dim1 == 11);
        REQUIRE_FALSE(hmpc::is_constant_of<decltype(dim1), hmpc::size>);

        auto [x, y] = shape;
        REQUIRE(x == hmpc::placeholder_extent);
        REQUIRE(y == 11);
    }

    SECTION("2D placeholder shape")
    {
        auto shape = hmpc::shape{hmpc::constants::placeholder, hmpc::size_constant_of<11>};
        auto size = shape.size();
        auto dim0 = shape.get<0>();
        auto dim1 = shape.get<1>();
        REQUIRE(shape.rank == 2);
        REQUIRE(size == 11);
        REQUIRE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == hmpc::placeholder_extent);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(dim1 == 11);
        REQUIRE(hmpc::is_constant_of<decltype(dim1), hmpc::size>);

        auto [x, y] = shape;
        REQUIRE(x == hmpc::placeholder_extent);
        REQUIRE(y == 11);
    }

    SECTION("2D placeholder shape")
    {
        auto shape = hmpc::shape{hmpc::constants::placeholder, hmpc::constants::placeholder};
        auto size = shape.size();
        auto dim0 = shape.get<0>();
        auto dim1 = shape.get<1>();
        REQUIRE(shape.rank == 2);
        REQUIRE(size == 1);
        REQUIRE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(dim0 == hmpc::placeholder_extent);
        REQUIRE(hmpc::is_constant_of<decltype(dim0), hmpc::size>);
        REQUIRE(dim1 == hmpc::placeholder_extent);
        REQUIRE(hmpc::is_constant_of<decltype(dim1), hmpc::size>);

        auto [x, y] = shape;
        REQUIRE(x == hmpc::placeholder_extent);
        REQUIRE(y == hmpc::placeholder_extent);
    }

    SECTION("element shape")
    {
        auto shape = hmpc::shape{hmpc::size_constant_of<42>, hmpc::size_constant_of<11>};
        using T = hmpc::default_limb;
        using Tx4 = hmpc::comp::vector<T, 4>;

        auto element_shape = hmpc::element_shape<Tx4>(shape);
        auto size = element_shape.size();
        REQUIRE(element_shape.rank == 3);
        REQUIRE(size == 42 * 11 * 4);
        REQUIRE(hmpc::is_constant_of<decltype(size), hmpc::size>);
        REQUIRE(element_shape.get(hmpc::constants::zero) == 42);
        REQUIRE(element_shape.get(hmpc::constants::one) == 11);
        REQUIRE(element_shape.get(hmpc::constants::two) == 4);
    }

    SECTION("unsqueeze")
    {
        auto shape = hmpc::shape{hmpc::size_constant_of<42>, hmpc::size_constant_of<11>};

        SECTION("unsqueeze(0)")
        {
            auto unsqueezed = hmpc::unsqueeze(shape, hmpc::constants::zero);
            auto [x, y, z] = unsqueezed;
            REQUIRE(x == hmpc::placeholder_extent);
            REQUIRE(hmpc::is_constant_of<decltype(x), hmpc::size>);
            REQUIRE(y == 42);
            REQUIRE(z == 11);
        }

        SECTION("unsqueeze(1)")
        {
            auto unsqueezed = hmpc::unsqueeze(shape, hmpc::constants::one);
            auto [x, y, z] = unsqueezed;
            REQUIRE(x == 42);
            REQUIRE(y == hmpc::placeholder_extent);
            REQUIRE(hmpc::is_constant_of<decltype(y), hmpc::size>);
            REQUIRE(z == 11);
        }

        SECTION("unsqueeze(2)")
        {
            auto unsqueezed = hmpc::unsqueeze(shape, hmpc::constants::two);
            auto [x, y, z] = unsqueezed;
            REQUIRE(x == 42);
            REQUIRE(y == 11);
            REQUIRE(z == hmpc::placeholder_extent);
            REQUIRE(hmpc::is_constant_of<decltype(z), hmpc::size>);
        }

        SECTION("unsqueeze(-1)")
        {
            auto unsqueezed = hmpc::unsqueeze(shape, hmpc::constants::minus_one);
            auto [x, y, z] = unsqueezed;
            REQUIRE(x == 42);
            REQUIRE(y == 11);
            REQUIRE(z == hmpc::placeholder_extent);
            REQUIRE(hmpc::is_constant_of<decltype(z), hmpc::size>);
        }

        SECTION("unsqueeze(0) with value")
        {
            auto unsqueezed = hmpc::unsqueeze(shape, hmpc::constants::zero, hmpc::size{2});
            auto [x, y, z] = unsqueezed;
            REQUIRE(x == 2);
            REQUIRE_FALSE(hmpc::is_constant_of<decltype(x), hmpc::size>);
            REQUIRE(y == 42);
            REQUIRE(z == 11);
        }

        SECTION("unsqueeze(-1) with value")
        {
            auto unsqueezed = hmpc::unsqueeze(shape, hmpc::constants::minus_one, hmpc::size_constant_of<4>);
            auto [x, y, z] = unsqueezed;
            REQUIRE(x == 42);
            REQUIRE(y == 11);
            REQUIRE(z == 4);
            REQUIRE(hmpc::is_constant_of<decltype(z), hmpc::size>);
        }
    }
}
