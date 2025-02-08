#pragma once

#include <hmpc/core/limb_traits.hpp>
#include <hmpc/ffi.hpp>
#include <hmpc/fmt.hpp>

#include <future>
#include <memory>
#include <stdexcept>
#include <system_error>

namespace hmpc::ffi
{
    [[noreturn]] inline void throw_exception(SendReceiveErrc errc) noexcept(false)
    {
        switch (errc)
        {
            case SendReceiveErrc::ok:
                throw std::logic_error("Ok is not an error");
            case SendReceiveErrc::invalid_handle:
                throw std::invalid_argument("handle");
            case SendReceiveErrc::invalid_pointer:
                throw std::invalid_argument("pointer");
            case SendReceiveErrc::invalid_size:
                throw std::invalid_argument("size");
            case SendReceiveErrc::invalid_communicator:
                throw std::invalid_argument("communicator");
            case SendReceiveErrc::invalid_metadata:
                throw std::invalid_argument("metadata");
            case SendReceiveErrc::version_mismatch:
                throw std::system_error(std::make_error_code(std::errc::wrong_protocol_type), "version mismatch");
            case SendReceiveErrc::feature_mismatch:
                throw std::system_error(std::make_error_code(std::errc::wrong_protocol_type), "feature mismatch");
            case SendReceiveErrc::channel_could_not_receive:
                throw std::system_error(std::make_error_code(std::future_errc::broken_promise), "cannot receive on channel");
            case SendReceiveErrc::channel_could_not_send:
                throw std::system_error(std::make_error_code(std::future_errc::broken_promise), "cannot send on channel");
            case SendReceiveErrc::connection_version_mismatch:
                throw std::system_error(std::make_error_code(std::errc::wrong_protocol_type), "connection version mismatch");
            case SendReceiveErrc::connection_transport_error:
                throw std::system_error(std::make_error_code(std::errc::protocol_error), "transport error");
            case SendReceiveErrc::connection_closed:
                throw std::system_error(std::make_error_code(std::errc::connection_reset), "connection closed");
            case SendReceiveErrc::connection_reset:
                throw std::system_error(std::make_error_code(std::errc::connection_reset), "connection reset");
            case SendReceiveErrc::connection_timed_out:
                throw std::system_error(std::make_error_code(std::errc::timed_out), "connection timed out");
            case SendReceiveErrc::connection_locally_closed:
                throw std::system_error(std::make_error_code(std::errc::connection_reset), "connection locally closed");
            case SendReceiveErrc::connections_exhausted:
                throw std::domain_error("connection identifiers");
            case SendReceiveErrc::application_closed:
                throw std::system_error(std::make_error_code(std::errc::connection_reset), "application closed");
            case SendReceiveErrc::stream_finished_early:
                throw std::system_error(std::make_error_code(std::errc::message_size), "stream finished early");
            case SendReceiveErrc::stream_reset:
                throw std::system_error(std::make_error_code(std::errc::connection_reset), "stream reset");
            case SendReceiveErrc::stream_stopped:
                throw std::system_error(std::make_error_code(std::errc::connection_reset), "stream stopped");
            case SendReceiveErrc::stream_closed:
                throw std::system_error(std::make_error_code(std::errc::connection_refused), "stream closed");
            case SendReceiveErrc::stream_illegal_ordered_read:
                throw std::system_error(std::make_error_code(std::errc::operation_not_permitted), "stream illegal ordered read");
            case SendReceiveErrc::stream_rejected:
                throw std::system_error(std::make_error_code(std::errc::connection_refused), "stream rejected");
            case SendReceiveErrc::stream_too_long:
                throw std::system_error(std::make_error_code(std::errc::message_size), "stream too long");
            case SendReceiveErrc::invalid_enum_value:
                throw std::domain_error("enum");
            case SendReceiveErrc::size_mismatch:
                throw std::system_error(std::make_error_code(std::errc::message_size), "size mismatch");
            case SendReceiveErrc::task_cancelled:
                throw std::system_error(std::make_error_code(std::future_errc::broken_promise), "task cancelled");
            case SendReceiveErrc::task_panicked:
                throw std::system_error(std::make_error_code(std::future_errc::broken_promise), "task panicked");
            case SendReceiveErrc::multiple_errors:
                throw std::runtime_error("multiple errors");
            case SendReceiveErrc::session_mismatch:
                throw std::runtime_error("session ID mismatch");
            case SendReceiveErrc::signature_verification:
                throw std::runtime_error("signature verification failed");
            case SendReceiveErrc::unknown_sender:
                throw std::runtime_error("unknown sender");
            case SendReceiveErrc::multiple_checks:
                throw std::runtime_error("multiple checks");
            case SendReceiveErrc::multiple_requests:
                throw std::runtime_error("multiple requests");
            case SendReceiveErrc::multiple_messages:
                throw std::runtime_error("multiple messages");
            case SendReceiveErrc::unknown_check:
                throw std::runtime_error("unknown check");
            case SendReceiveErrc::inconsistent_signature_verification:
                throw std::runtime_error("signature verification failed (consistency check)");
            case SendReceiveErrc::inconsistent_collective_communication:
                throw std::runtime_error("inconsistent collective communication");
        }
    }
}

namespace hmpc::net
{
    using party_id = hmpc::ffi::PartyID;
    using message_datatype = hmpc::ffi::MessageDatatype;
    using message_size = hmpc::ffi::MessageSize;
    using data_ptr = hmpc::ffi::NullableDataPtr;
    using statistics = hmpc::ffi::NetworkStatistics;

    namespace traits
    {
        template<typename T>
        struct message_datatype_of
        {
            using value_type = hmpc::net::message_datatype;

            static constexpr bool is_little_endian = []()
            {
                constexpr auto one = T{1};
                constexpr auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(one);
                static_assert(bytes[0] == std::byte{0} or bytes[0] == std::byte{1});
                return bytes[0] == std::byte{1};
            }();

            static constexpr value_type value = []()
            {
                constexpr auto bit_tag = value_type{hmpc::core::limb_traits<T>::bit_size};

                static_assert(std::numeric_limits<value_type>::radix == 2);
                static_assert(std::numeric_limits<value_type>::digits > 0);
                static_assert(std::numeric_limits<value_type>::is_integer);
                static_assert(not std::numeric_limits<value_type>::is_signed);

                return value_type{is_little_endian} | bit_tag;
            }();
        };

        template<typename T>
        constexpr auto message_datatype_of_v = message_datatype_of<T>::value;
    }

    namespace detail
    {
        template<typename R>
            requires std::ranges::contiguous_range<R> and std::ranges::sized_range<R>
        constexpr auto to_ffi(R&& range) noexcept
        {
            return hmpc::ffi::Span
            {
                .data = std::ranges::data(range),
                .len = std::ranges::size(range),
            };
        }

        template<typename T>
        constexpr auto to_single_ffi(T&& value) noexcept
        {
            return hmpc::ffi::Span
            {
                .data = std::addressof(value),
                .len = 1,
            };
        }

        template<typename R>
            requires std::ranges::contiguous_range<R> and std::ranges::sized_range<R>
        constexpr auto to_outer_ffi(R&& range) noexcept
        {
            return hmpc::ffi::Span2d
            {
                .data = std::ranges::data(range),
                .outer_extent = std::ranges::size(range),
                .inner_extent = 1,
            };
        }

        template<std::size_t Outer, std::size_t Inner>
        constexpr auto to_2d_ffi(std::array<std::array<data_ptr, Inner>, Outer>& array) noexcept
        {
            return hmpc::ffi::Span2d
            {
                .data = array[0].data(),
                .outer_extent = Outer,
                .inner_extent = Inner,
            };
        }
    }
}

namespace hmpc
{
    template<hmpc::net::party_id Id>
    using party_constant = hmpc::constant<hmpc::net::party_id, Id>;

    template<hmpc::net::party_id Id>
    constexpr party_constant<Id> party_constant_of = {};
}

/// Format hmpc::net::statistics
///
/// The format specifier is
/// ```
/// n(optional) h(optional) (u|U)(optional)
/// ```
/// where
/// - 'n': omits outer braces
/// - 'h': converts the numbers to nice "human-readable" representations
/// - 'u': adds units ("b" for bits)
/// - 'U': adds units ("B" for bytes)
///
/// # Examples
/// ```cpp
/// auto stats = hmpc::net::statistics{.sent=43008000, .received=10, .rounds=1};
/// format("{}", stats); // "{sent: 43008000, received: 10, rounds: 1}"
/// format("{h}", stats); // "{sent: 42000 Ki, received: 10, rounds: 1}"
/// format("{hu}", stats); // "{sent: 336000 Kib, received: 80 b, rounds: 1}"
/// format("{nhU}", stats); // "sent: 42000 KiB, received: 10 B, rounds: 1"
/// ```
template<typename Char>
struct HMPC_FMTLIB::formatter<hmpc::net::statistics, Char>
{
    static constexpr bool is_specialized = true;

    bool no_braces = false;
    bool human_readable = false;
    hmpc::fmt::units::binary units = hmpc::fmt::units::binary::none;

    constexpr auto parse(auto& ctx)
    {
        using hmpc::fmt::units::binary;

        auto it = ctx.begin();
        if (it == ctx.end())
        {
            return it;
        }

        if (*it == Char{'n'})
        {
            no_braces = true;
            ++it;
        }
        if (it != ctx.end() and *it == Char{'h'})
        {
            human_readable = true;
            ++it;
        }
        if (it != ctx.end() and *it == Char{'u'})
        {
            units = binary::bit;
            ++it;
        }
        else if (it != ctx.end() and *it == Char{'U'})
        {
            units = binary::byte;
            ++it;
        }
        if (it != ctx.end() and *it != Char{'}'})
        {
            throw HMPC_FMTLIB::format_error("Invalid format specifier");
        }

        return it;
    }

    auto format(hmpc::net::statistics value, auto& ctx) const
    {
        using hmpc::fmt::units::binary;

        auto out = ctx.out();
        if (not no_braces)
        {
            out = HMPC_FMTLIB::format_to(out, "{{");
        }

        out = HMPC_FMTLIB::format_to(out, "sent: ");
        out = hmpc::fmt::format_maybe_human_readable_byte_size(out, human_readable, value.sent, units);
        out = HMPC_FMTLIB::format_to(out, ", received: ");
        out = hmpc::fmt::format_maybe_human_readable_byte_size(out, human_readable, value.received, units);

        out = HMPC_FMTLIB::format_to(out, ", rounds: {}", value.rounds);

        if (not no_braces)
        {
            out = HMPC_FMTLIB::format_to(out, "}}");
        }
        return out;
    }
};
