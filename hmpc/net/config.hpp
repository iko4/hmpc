#pragma once

#include <hmpc/net/ffi.hpp>

namespace hmpc::net
{
    struct config
    {
    private:
        explicit config(hmpc::ffi::Config* config)
            : handle(config)
        {
        }
        using deleter_type = decltype([](hmpc::ffi::Config* config)
        {
            if (config != nullptr)
            {
                hmpc::ffi::hmpc_ffi_net_config_free(config);
            }
        });
        std::unique_ptr<hmpc::ffi::Config, deleter_type> handle;

    public:
        hmpc::ffi::Config* release() &&
        {
            return handle.release();
        }

        /// Read a config file from the default config path
        ///
        /// This tries to find a config path in the following order
        /// - the environment
        /// - the hard-coded fallback config path
        static config read(std::nullptr_t = nullptr)
        {
            return config{hmpc::ffi::hmpc_ffi_net_config_read(nullptr)};
        }

        /// Read a config file
        ///
        /// This tries to find a config path in the following order
        /// - the provided path (if not `nullptr`)
        /// - the environment
        /// - the hard-coded fallback config path
        static config read(char const* path)
        {
            return config{hmpc::ffi::hmpc_ffi_net_config_read(path)};
        }

        /// Read a config file
        ///
        /// This tries to find a config path in the following order
        /// - the provided path
        static config read(std::string const& path)
        {
            return config{hmpc::ffi::hmpc_ffi_net_config_read(path.c_str())};
        }

        /// Read a config file
        ///
        /// This tries to find a config path in the following order
        /// - the environment
        /// - the provided path (if not `nullptr`)
        /// - the hard-coded fallback config path
        static config read_env(char const* path)
        {
            return config{hmpc::ffi::hmpc_ffi_net_config_read_env(path)};
        }

        /// Read a config file
        ///
        /// This tries to find a config path in the following order
        /// - the environment
        /// - the provided path
        static config read_env(std::string const& path)
        {
            return config{hmpc::ffi::hmpc_ffi_net_config_read_env(path.c_str())};
        }

        /// Update the session ID of a config to values set by environment variables (if possible)
        bool session_from_env()
        {
            return hmpc::ffi::hmpc_ffi_net_config_session_from_env(handle.get());
        }
    };
}
