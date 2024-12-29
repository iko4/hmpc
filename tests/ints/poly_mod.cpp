#include "catch_helpers.hpp"

#include <hmpc/expr/tensor.hpp>
#include <hmpc/expr/number_theoretic_transform.hpp>
#include <hmpc/ints/poly_mod.hpp>
#include <hmpc/ints/literals.hpp>

TEST_CASE("Integer polynomials modulo", "[ints][poly][mod]")
{
    using namespace hmpc::ints::literals;
    constexpr auto p = 0x2faeadbe7a0195c011ac195ad10269830e8001_int; // 1'063'351'684'119'354'455'646'439'919'975'158'459'206'107'137

    constexpr hmpc::size N = 1 << 14;
    using R = hmpc::ints::poly_mod<p, N, hmpc::ints::coefficient_representation>;
    using mod = R::element_type;
    using integer = mod::unsigned_type;
    using limb = mod::limb_type;

    auto shape = hmpc::shape{2};
    using index_type = hmpc::traits::dynamic_index_t<hmpc::traits::element_shape_t<R, decltype(shape)>>;

    auto x = hmpc::comp::make_tensor<R>(shape);

    hmpc::comp::queue queue{sycl::queue(sycl::cpu_selector_v)};

    SECTION("x = 0")
    {
        {
            hmpc::comp::host_accessor x_elements(x, hmpc::access::discard_write);
            for (hmpc::size i = 0; i < x.element_shape().size(); ++i)
            {
                x_elements[i] = mod{};
            }
        }

        auto y = hmpc::expr::number_theoretic_transform(hmpc::expr::tensor(x));

        REQUIRE(y.shape().rank == 1);
        REQUIRE(y.shape().get(hmpc::constants::zero) == 2);

        auto [ntt_x, z] = queue(y, hmpc::expr::inverse_number_theoretic_transform(y));
        REQUIRE(ntt_x.shape().rank == 1);
        REQUIRE(ntt_x.shape().get(hmpc::constants::zero) == 2);
        REQUIRE(z.shape().rank == 1);
        REQUIRE(z.shape().get(hmpc::constants::zero) == 2);

        hmpc::comp::host_accessor z_elements(z, hmpc::access::read);
        hmpc::comp::host_accessor ntt_x_elements(ntt_x, hmpc::access::read);
        for (hmpc::size i = 0; i < x.element_shape().size(); ++i)
        {
            mod z = z_elements[i];
            mod ntt_x = ntt_x_elements[i];
            CHECK(z == mod{});
            CHECK(ntt_x == mod{});
        }
    }

    SECTION("x = i")
    {
        auto element_size = shape.size();
        {
            hmpc::comp::host_accessor x_elements(x, hmpc::access::read_write);
            for (hmpc::size j = 0; j < element_size; ++j)
            {
                for (hmpc::size i = 0; i < N; ++i)
                {
                    x_elements[index_type{j, i}] = mod{hmpc::ints::ubigint<32>{static_cast<limb>(i)}};
                }
            }
        }

        auto y = hmpc::expr::number_theoretic_transform(hmpc::expr::tensor(x));
        auto ntt_y = hmpc::expr::inverse_number_theoretic_transform_expression{y};
        REQUIRE(std::same_as<decltype(y.inner), decltype(hmpc::expr::inverse_number_theoretic_transform(y))>);

        REQUIRE(y.shape().rank == 1);
        REQUIRE(y.shape().get(hmpc::constants::zero) == 2);
        REQUIRE(static_cast<integer>(y.root) == 0x001b'5b9f'69e9'7ca1'4b57'556d'08f4'c26f'0368'4dc1_int);     // 610101584057729959778752636215942153358560705
        REQUIRE(static_cast<integer>(ntt_y.root) == 0x0007'54dd'5b37'cdc9'02de'aaf1'680f'db42'e9f6'e083_int); // 163497972060569513473824561702379370359218307

        auto [ntt_x, z] = queue(y, ntt_y);
        REQUIRE(ntt_x.shape().rank == 1);
        REQUIRE(ntt_x.shape().get(hmpc::constants::zero) == 2);
        REQUIRE(z.shape().rank == 1);
        REQUIRE(z.shape().get(hmpc::constants::zero) == 2);

        hmpc::comp::host_accessor x_elements(x, hmpc::access::read);
        hmpc::comp::host_accessor z_elements(z, hmpc::access::read);
        hmpc::comp::host_accessor ntt_x_elements(ntt_x, hmpc::access::read);
        for (hmpc::size j = 0; j < element_size; ++j)
        {
            REQUIRE(static_cast<integer>(static_cast<mod>(ntt_x_elements[index_type{j, 0}])) == 0x7bf0fe81e23706642244f0cf8fb65af112666_int); // 172'749'075'775'567'100'571'495'461'235'136'253'733'709'414
            REQUIRE(static_cast<integer>(static_cast<mod>(ntt_x_elements[index_type{j, 1}])) == 0xb8129497504c45e2fab6f665863b8d0cdb431_int); // 256'559'731'287'316'797'106'230'355'761'951'944'760'734'769
            REQUIRE(static_cast<integer>(static_cast<mod>(ntt_x_elements[index_type{j, 2}])) == 0x25b2015ae56339a59389c056ae782f1e58b697_int); // 840'634'020'345'421'428'124'702'238'523'744'314'730'264'215
            REQUIRE(static_cast<integer>(static_cast<mod>(ntt_x_elements[index_type{j, 10}])) == 0x2433793de67b5574558745981a36837082b73e_int); // 807'310'810'175'657'166'349'942'851'846'487'741'206'542'142
            REQUIRE(static_cast<integer>(static_cast<mod>(ntt_x_elements[index_type{j, 19}])) == 0x1cdb8c2cce3758d6acd87bf78549948e08faba_int); // 643'546'155'266'049'411'803'220'275'137'192'086'018'259'642
        }
        for (hmpc::size i = 0; i < x.element_shape().size(); ++i)
        {
            mod x = x_elements[i];
            mod z = z_elements[i];
            CHECK(x == z);
        }
    }
}
