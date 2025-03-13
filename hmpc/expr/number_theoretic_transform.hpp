#pragma once

#include <hmpc/comp/queue.hpp>
#include <hmpc/core/countr_zero.hpp>
#include <hmpc/detail/type_id.hpp>
#include <hmpc/expr/expression.hpp>
#include <hmpc/ints/num/theory/root_of_unity.hpp>
#include <hmpc/ints/poly.hpp>

namespace hmpc::expr
{
    template<typename T, bool Inverse>
    struct basic_number_theoretic_transform
    {
        using value_type = T;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using limb_type = hmpc::traits::limb_type_t<value_type>;
        static constexpr auto vector_size = hmpc::traits::vector_size<value_type>{};
        static constexpr auto iteration_count = hmpc::core::countr_zero(vector_size);
        static constexpr auto limb_size = hmpc::traits::limb_size<value_type>{};
        static constexpr auto limb_bit_size = limb_type::bit_size;
        static constexpr auto root = []()
        {
            constexpr auto phi = hmpc::ints::num::theory::root_of_unity(hmpc::constant_of<value_type::modulus>, hmpc::size_constant_of<2 * vector_size>);
            if constexpr (Inverse)
            {
                return invert(phi);
            }
            else
            {
                return phi;
            }
        }();

        static auto& get_roots(auto&, auto& get_extra_tensor) HMPC_NOEXCEPT
        {
            constexpr std::string_view tag = []()
            {
                if constexpr (Inverse)
                {
                    return "hmpc::expr::inverse_number_theoretic_transform";
                }
                else
                {
                    return "hmpc::expr::number_theoretic_transform";
                }
            }();

            return get_extra_tensor(hmpc::comp::tensor_lookup_key_view{hmpc::detail::type_id_of<value_type>(), limb_bit_size, limb_size, 0, vector_size, tag}, [&]()
            {
                static_assert(hmpc::detail::has_single_bit(vector_size));
                static_assert(hmpc::detail::bit_width(vector_size) <= 32);
                constexpr auto bits = hmpc::size_constant_of<iteration_count>;

                auto tensor = hmpc::comp::tensor<element_type, vector_size>({});
                {
                    hmpc::comp::host_accessor roots(tensor, hmpc::access::discard_write);

                    constexpr auto normalization = invert(element_type{hmpc::ints::ubigint<32>{limb_type{vector_size}}});
                    if constexpr (Inverse)
                    {
                        roots[0] = normalization;
                    }
                    else
                    {
                        roots[0] = hmpc::ints::integer_traits<element_type>::one;
                    }
                    roots[hmpc::detail::bit_reverse(1, bits)] = root;

                    auto power = root;
                    for (hmpc::size i = 2; i < vector_size; ++i)
                    {
                        power *= root;
                        auto j = hmpc::detail::bit_reverse(i, bits);
                        if (Inverse and j == 1) // the power of phi for the last iteration of the inverse transform is pre-multiplied with the normalization factor
                        {
                            roots[j] = power * normalization;
                        }
                        else
                        {
                            roots[j] = power;
                        }
                    }
                }

                return hmpc::comp::tensor_lookup_value{std::move(tensor)};
            }).tensor;
        }

        static void inner_transform(sycl::queue& sycl_queue, hmpc::comp::tensor<element_type, vector_size, hmpc::dynamic_extent>& scratch_buffer, hmpc::comp::tensor<element_type, vector_size>& roots, hmpc::size element_size)
            requires (Inverse)
        {
            hmpc::iter::for_each(hmpc::range(hmpc::size_constant_of<1>, hmpc::iter::prev(iteration_count)), [&](auto iteration)
            {
                sycl_queue.submit([&](auto& handler)
                {
                    auto read_write = hmpc::comp::device_accessor(scratch_buffer, handler, hmpc::access::read_write);
                    auto psis = hmpc::comp::device_accessor(roots, handler, hmpc::access::read);

                    handler.parallel_for(sycl::range{vector_size / 2 * element_size}, [=](hmpc::size id)
                    {
                        hmpc::size tid = id / element_size;
                        hmpc::size i = id % element_size;
                        // Inferred from [1]
                        // Let 0 <= iteration < log2(vector_size)
                        // Let length = vector_size / 2^(iteration + 1)
                        // Let 0 <= tid < vector_size / 2
                        // Let step = (vector_size / length) / 2
                        // Let psi_step = tid / step
                        // Let target_idx = (psi_step * step * 2) + (tid mod step)
                        // Let step_group = length + psi_step
                        // Let psi = psis[step_group]
                        // Let u = data[target_idx]
                        // Let v = data[target_idx + step]
                        //
                        // ### Data indices
                        // Let index = (tid, i)
                        //     0 <= i < element_size
                        //     0 <= tid < vector_size / 2
                        // Read/write (target_idx, i)
                        // => i + target_idx * element_size
                        // Read/write (target_idx + step, i)
                        // => i + (target_idx + step) * element_size
                        constexpr hmpc::size length = vector_size >> (iteration + 1);
                        constexpr hmpc::size step = (vector_size / length) / 2;
                        hmpc::size psi_step = tid / step;
                        hmpc::size target_idx = (psi_step * step * 2) + (tid % step);
                        hmpc::size step_group = length + psi_step;
                        auto psi = psis[step_group];

                        auto index = i + target_idx * element_size;
                        auto index_step = i + (target_idx + step) * element_size;

                        element_type u = read_write[index];
                        element_type v = read_write[index_step];

                        read_write[index] = u + v;
                        read_write[index_step] = (u - v) * psi;
                    });
                });
            });
        }

        static void inner_transform(sycl::queue& sycl_queue, hmpc::comp::tensor<element_type, vector_size, hmpc::dynamic_extent>& scratch_buffer, hmpc::comp::tensor<element_type, vector_size>& roots, hmpc::size element_size)
            requires (not Inverse)
        {
            hmpc::iter::for_each(hmpc::range(hmpc::size_constant_of<1>, hmpc::iter::prev(iteration_count)), [&](auto iteration)
            {
                sycl_queue.submit([&](auto& handler)
                {
                    auto read_write = hmpc::comp::device_accessor(scratch_buffer, handler, hmpc::access::read_write);
                    auto psis = hmpc::comp::device_accessor(roots, handler, hmpc::access::read);

                    handler.parallel_for(sycl::range{vector_size / 2 * element_size}, [=](hmpc::size id)
                    {
                        hmpc::size tid = id / element_size;
                        hmpc::size i = id % element_size;
                        // [1, Algorithm 8]
                        // Let 0 <= iteration < log2(vector_size)
                        // Let length = 2^iteration
                        // Let 0 <= tid < vector_size / 2
                        // Let step = (vector_size / length) / 2
                        // Let psi_step = tid / step
                        // Let target_idx = (psi_step * step * 2) + (tid mod step)
                        // Let step_group = length + psi_step
                        // Let psi = psis[step_group]
                        // Let u = data[target_idx]
                        // Let v = data[target_idx + step]
                        //
                        // ### Data indices
                        // Let index = (tid, i)
                        //     0 <= i < element_size
                        //     0 <= tid < vector_size / 2
                        // Read/write (target_idx, i)
                        // => i + target_idx * element_size
                        // Read/write (target_idx + step, i)
                        // => i + (target_idx + step) * element_size
                        constexpr hmpc::size length = 1 << iteration;
                        constexpr hmpc::size step = (vector_size / length) / 2;
                        hmpc::size psi_step = tid / step;
                        hmpc::size target_idx = (psi_step * step * 2) + (tid % step);
                        hmpc::size step_group = length + psi_step;
                        auto psi = psis[step_group];

                        auto index = i + target_idx * element_size;
                        auto index_step = i + (target_idx + step) * element_size;

                        element_type u = read_write[index];
                        element_type v = read_write[index_step];
                        v *= psi;

                        read_write[index] = u + v;
                        read_write[index_step] = u - v;
                    });
                });
            });
        }
    };

    template<hmpc::expression E, bool Inverse>
    struct number_theoretic_transform_base
        : public basic_number_theoretic_transform<typename E::value_type, Inverse>
        , public enable_caching
    {
        using inner_type = E;
        using value_type = inner_type::value_type;
        using element_type = hmpc::traits::element_type_t<value_type>;
        using limb_type = hmpc::traits::limb_type_t<value_type>;
        using shape_type = inner_type::shape_type;

        static constexpr auto arity = hmpc::size_constant_of<1>;
        using is_complex = void;

        inner_type inner;

        constexpr number_theoretic_transform_base(inner_type inner) HMPC_NOEXCEPT
            : inner(inner)
        {
        }

        constexpr inner_type const& get(hmpc::size_constant<0>) const noexcept
        {
            return inner;
        }

        static constexpr hmpc::access::once_tag access(hmpc::size_constant<0>) noexcept
        {
            return {};
        }

        constexpr decltype(auto) shape() const HMPC_NOEXCEPT
        {
            return inner.shape();
        }
    };

    /// #### Algorithm reference
    /// - [1] Özgün Özerk, Can Elgezen, Ahmet Can Mert, Erdinç Öztürk, Erkay Savaş: "Efficient number theoretic transform implementation on GPU for homomorphic encryption." The Journal of Supercomputing, Volume 78, Number 2, 2022. [Link](https://eprint.iacr.org/2021/124.pdf), accessed 2024-02-16.
    template<hmpc::expression E>
    struct number_theoretic_transform_expression : public number_theoretic_transform_base<E, false>
    {
        using base = number_theoretic_transform_base<E, false>;

        using inner_type = base::inner_type;
        using inner_value_type = typename base::value_type;
        using typename base::element_type;

        static_assert(inner_value_type::representation == hmpc::ints::coefficient_representation);
        using value_type = hmpc::ints::traits::number_theoretic_transform_type_t<inner_value_type>;

        using base::inner;
        using base::iteration_count;
        using base::vector_size;

        using base::operator();

        constexpr number_theoretic_transform_expression(E inner) HMPC_NOEXCEPT
            : base(inner)
        {
        }

        constexpr auto operator()(auto& sycl_queue, auto& get_state, auto& get_capability_data, auto& make_capabilities, auto& tensor, auto& get_extra_tensor) const HMPC_NOEXCEPT
        {
            auto& roots = base::get_roots(sycl_queue, get_extra_tensor);

            auto scratch_buffer_shape = hmpc::shape{hmpc::size_constant_of<vector_size>, hmpc::dynamic_value(inner.shape().size())};

            auto scratch_buffer = hmpc::comp::make_tensor<element_type>(scratch_buffer_shape);

            sycl_queue.submit([&](auto& handler)
            {
                auto state = get_state(handler).get(hmpc::constants::zero);
                auto write = hmpc::comp::device_accessor(scratch_buffer, handler, hmpc::access::discard_write);
                auto psis = hmpc::comp::device_accessor(roots, handler, hmpc::access::read);
                auto element_shape = hmpc::expr::element_shape(inner);
                auto element_size = inner.shape().size();
                auto capability_data = get_capability_data(element_shape);

                handler.parallel_for(sycl::range{element_size * vector_size / 2}, [=](hmpc::size id)
                {
                    hmpc::size i = id / (vector_size / 2);
                    hmpc::size tid = id % (vector_size / 2);
                    // [1, Algorithm 8]
                    // Let 0 <= iteration < log2(vector_size) := 0
                    // Let length = 2^iteration := 1
                    // Let 0 <= tid < vector_size / 2
                    // Let step = (vector_size / length) / 2 := vector_size / 2
                    // Let psi_step = tid / step := 0
                    // Let target_idx = (psi_step * step * 2) + (tid mod step) := tid
                    // Let step_group = length + psi_step := 1
                    // Let psi = psis[step_group]
                    // Let u = data[target_idx]
                    // Let v = data[target_idx + step]
                    //
                    // ### Data indices
                    // Let index = (i, tid)
                    //     0 <= i < element_size
                    //     0 <= tid < vector_size / 2
                    // Read (i, tid)
                    //  => tid + i * vector_size
                    // Read (i, tid + step)
                    //  => tid + step + i * vector_size
                    // Write (tid, i)
                    // => i + tid * element_size
                    // Write (tid + step, i)
                    // => i + (tid + step) * element_size
                    constexpr hmpc::size step = vector_size / 2;
                    auto psi = psis[1];

                    auto [index, index_step] = [&]()
                    {
                        if constexpr (hmpc::expr::same_element_shape<inner_type>)
                        {
                            return std::make_pair(tid + i * vector_size, tid + step + i * vector_size);
                        }
                        else
                        {
                            return std::make_pair(
                                hmpc::from_linear_index(tid + i * vector_size, element_shape),
                                hmpc::from_linear_index(tid + step + i * vector_size, element_shape)
                            );
                        }
                    }();
                    auto write_index = i + tid * element_size;
                    auto write_index_step = i + (tid + step) * element_size;
                    auto capabilities = make_capabilities(capability_data, index, element_shape);

                    auto u = inner_type::operator()(state, index, capabilities);
                    auto v = inner_type::operator()(state, index_step, capabilities);
                    v *= psi;

                    write[write_index] = u + v;
                    write[write_index_step] = u - v;
                });
            });

            base::inner_transform(sycl_queue, scratch_buffer, roots, inner.shape().size());

            return sycl_queue.submit([&](auto& handler)
            {
                auto read = hmpc::comp::device_accessor(scratch_buffer, handler, hmpc::access::read);
                auto write = hmpc::comp::device_accessor(tensor, handler, hmpc::access::discard_write);
                auto psis = hmpc::comp::device_accessor(roots, handler, hmpc::access::read);
                auto element_size = inner.shape().size();

                handler.parallel_for(sycl::range{vector_size / 2 * element_size}, [=](hmpc::size id)
                {
                    hmpc::size tid = id / element_size;
                    hmpc::size i = id % element_size;
                    // [1, Algorithm 8]
                    // Let 0 <= iteration < log2(vector_size) := log2(vector_size) - 1
                    // Let length = 2^iteration := vector_size / 2
                    // Let 0 <= tid < vector_size / 2
                    // Let step = (vector_size / length) / 2 := 1
                    // Let psi_step = tid / step := tid
                    // Let target_idx = (psi_step * step * 2) + (tid mod step) := 2 * tid
                    // Let step_group = length + psi_step := vector_size / 2 + tid
                    // Let psi = psis[step_group]
                    // Let u = data[target_idx]
                    // Let v = data[target_idx + step]
                    //
                    // ### Data indices
                    // Let index = (tid, i)
                    //     0 <= i < element_size
                    //     0 <= tid < vector_size / 2
                    // Read (target_idx, i)
                    //  => i + target_idx * element_size
                    // Read (target_idx + step, i)
                    //  => i + (target_idx + step) * element_size
                    // Write (i, target_idx)
                    // => target_idx + i * vector_size
                    // Write (i, target_idx + step)
                    // => target_idx + step + i * vector_size
                    constexpr hmpc::size length = vector_size / 2;
                    constexpr hmpc::size step = 1;
                    hmpc::size target_idx = 2 * tid;
                    hmpc::size step_group = length + tid;
                    auto psi = psis[step_group];

                    auto index = i + target_idx * element_size;
                    auto index_step = i + (target_idx + step) * element_size;
                    auto write_index = target_idx + i * vector_size;
                    auto write_index_step = target_idx + step + i * vector_size;

                    element_type u = read[index];
                    element_type v = read[index_step];
                    v *= psi;

                    write[write_index] = u + v;
                    write[write_index_step] = u - v;
                });
            });
        }
    };

    template<hmpc::expression E>
    struct inverse_number_theoretic_transform_expression : public number_theoretic_transform_base<E, true>
    {
        using base = number_theoretic_transform_base<E, true>;

        using inner_type = base::inner_type;
        using inner_value_type = typename base::value_type;
        using typename base::element_type;

        static_assert(inner_value_type::representation == hmpc::ints::number_theoretic_transform_representation);
        using value_type = hmpc::ints::traits::coefficient_type_t<inner_value_type>;

        using base::inner;
        using base::iteration_count;
        using base::vector_size;

        using base::operator();

        constexpr inverse_number_theoretic_transform_expression(E inner) HMPC_NOEXCEPT
            : base(inner)
        {
        }

        constexpr auto operator()(auto& sycl_queue, auto& get_state, auto& get_capability_data, auto& make_capabilities, auto& tensor, auto& get_extra_tensor) const HMPC_NOEXCEPT
        {
            auto& roots = base::get_roots(sycl_queue, get_extra_tensor);

            auto scratch_buffer_shape = hmpc::shape{hmpc::size_constant_of<vector_size>, hmpc::dynamic_value(inner.shape().size())};

            auto scratch_buffer = hmpc::comp::make_tensor<element_type>(scratch_buffer_shape);

            sycl_queue.submit([&](auto& handler)
            {
                auto state = get_state(handler).get(hmpc::constants::zero);
                auto write = hmpc::comp::device_accessor(scratch_buffer, handler, hmpc::access::discard_write);
                auto psis = hmpc::comp::device_accessor(roots, handler, hmpc::access::read);
                auto element_shape = hmpc::expr::element_shape(inner);
                auto element_size = inner.shape().size();
                auto capability_data = get_capability_data(element_shape);

                handler.parallel_for(sycl::range{element_size * vector_size / 2}, [=](hmpc::size id)
                {
                    hmpc::size i = id / (vector_size / 2);
                    hmpc::size tid = id % (vector_size / 2);
                    // Inferred from [1]
                    // Let 0 <= iteration < log2(vector_size) := 0
                    // Let length = vector_size / 2^(iteration + 1) := vector_size / 2
                    // Let 0 <= tid < vector_size / 2
                    // Let step = (vector_size / length) / 2 := 1
                    // Let psi_step = tid / step := tid
                    // Let target_idx = (psi_step * step * 2) + (tid mod step) := 2 * tid
                    // Let step_group = length + psi_step := vector_size / 2 + tid
                    // Let psi = psis[step_group]
                    // Let u = data[target_idx]
                    // Let v = data[target_idx + step]
                    //
                    // ### Data indices
                    // Let index = (i, tid)
                    //     0 <= i < element_size
                    //     0 <= tid < vector_size / 2
                    // Read (i, target_idx)
                    //  => target_idx + i * vector_size
                    // Read (i, target_idx + step)
                    //  => target_idx + step + i * vector_size
                    // Write (target_idx, i)
                    // => i + target_idx * element_size
                    // Write (target_idx + step, i)
                    // => i + (target_idx + step) * element_size
                    constexpr hmpc::size length = vector_size / 2;
                    constexpr hmpc::size step = 1;
                    hmpc::size target_idx = 2 * tid;
                    hmpc::size step_group = length + tid;
                    auto psi = psis[step_group];

                    auto [index, index_step] = [&]()
                    {
                        if constexpr (hmpc::expr::same_element_shape<inner_type>)
                        {
                            return std::make_pair(target_idx + i * vector_size, target_idx + step + i * vector_size);
                        }
                        else
                        {
                            return std::make_pair(
                                hmpc::from_linear_index(target_idx + i * vector_size, element_shape),
                                hmpc::from_linear_index(target_idx + step + i * vector_size, element_shape)
                            );
                        }
                    }();
                    auto write_index = i + target_idx * element_size;
                    auto write_index_step = i + (target_idx + step) * element_size;
                    auto capabilities = make_capabilities(capability_data, index, element_shape);

                    auto u = inner_type::operator()(state, index, capabilities);
                    auto v = inner_type::operator()(state, index_step, capabilities);

                    write[write_index] = u + v;
                    write[write_index_step] = (u - v) * psi;
                });
            });

            base::inner_transform(sycl_queue, scratch_buffer, roots, inner.shape().size());

            return sycl_queue.submit([&](auto& handler)
            {
                auto read = hmpc::comp::device_accessor(scratch_buffer, handler, hmpc::access::read);
                auto write = hmpc::comp::device_accessor(tensor, handler, hmpc::access::discard_write);
                auto psis = hmpc::comp::device_accessor(roots, handler, hmpc::access::read);
                auto element_size = inner.shape().size();

                handler.parallel_for(sycl::range{vector_size / 2 * element_size}, [=](hmpc::size id)
                {
                    hmpc::size tid = id / element_size;
                    hmpc::size i = id % element_size;
                    // Inferred from [1]
                    // Let 0 <= iteration < log2(vector_size) := log2(vector_size) - 1
                    // Let length = vector_size / 2^(iteration + 1) := 1
                    // Let 0 <= tid < vector_size / 2
                    // Let step = (vector_size / length) / 2 := vector_size / 2
                    // Let psi_step = tid / step := 0
                    // Let target_idx = (psi_step * step * 2) + (tid mod step) := tid
                    // Let step_group = length + psi_step := 1
                    // Let psi = psis[step_group]
                    // Let u = data[target_idx]
                    // Let v = data[target_idx + step]
                    //
                    // ### Data indices
                    // Let index = (tid, i)
                    //     0 <= i < element_size
                    //     0 <= tid < vector_size / 2
                    // Read (tid, i)
                    //  => i + tid * element_size
                    // Read (tid + step, i)
                    //  => i + (tid + step) * element_size
                    // Write (i, tid)
                    // => tid + i * vector_size
                    // Write (i, tid + step)
                    // => tid + step + i * vector_size
                    constexpr hmpc::size step = vector_size / 2;
                    auto psi = psis[1];

                    auto index = i + tid * element_size;
                    auto index_step = i + (tid + step) * element_size;
                    auto write_index = tid + i * vector_size;
                    auto write_index_step = tid + step + i * vector_size;

                    element_type u = read[index];
                    element_type v = read[index_step];
                    // multiplication of final results by vector_size^{-1}
                    // psi = psis[1] is already pre-multiplied with vector_size^{-1}
                    write[write_index] = (u + v) * psis[0];
                    write[write_index_step] = (u - v) * psi;
                });
            });
        }
    };

    template<hmpc::expression E>
    constexpr auto number_theoretic_transform(E e)
    {
        return number_theoretic_transform_expression<E>{e};
    }

    template<hmpc::expression E>
    constexpr auto number_theoretic_transform(inverse_number_theoretic_transform_expression<E> e)
    {
        return e.get(hmpc::constants::zero);
    }

    template<hmpc::expression_tuple E>
    constexpr auto number_theoretic_transform(E e)
    {
        return hmpc::iter::unpack(hmpc::range(E::arity), [&](auto... i)
        {
            return E::from_parts(number_theoretic_transform(e.get(i))...);
        });
    }

    template<hmpc::expression E>
    constexpr auto inverse_number_theoretic_transform(E e)
    {
        return inverse_number_theoretic_transform_expression<E>{e};
    }

    template<hmpc::expression E>
    constexpr auto inverse_number_theoretic_transform(number_theoretic_transform_expression<E> e)
    {
        return e.get(hmpc::constants::zero);
    }

    template<hmpc::expression_tuple E>
    constexpr auto inverse_number_theoretic_transform(E e)
    {
        return hmpc::iter::unpack(hmpc::range(E::arity), [&](auto... i)
        {
            return E::from_parts(inverse_number_theoretic_transform(e.get(i))...);
        });
    }
}
