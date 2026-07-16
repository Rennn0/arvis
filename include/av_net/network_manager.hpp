#pragma once
#include <cstdint>

namespace mjNet
{
    /// @brief result of a request
    enum class response_status : std::uint8_t
    {
        Ok,
        Failed,
        Canceled
    };

    /// @brief HTTP method to use for a request
    enum class request_method : std::uint8_t
    {
        get,
        post,
    };

    /// @brief abstractions over some http methods, uses libcurl
    class NetworkManager
    {
    public:
        /// @brief global init
        NetworkManager();

        /// @brief global cleanup
        ~NetworkManager();

        /// @brief sends get request and writes response data into folder
        /// @param url
        response_status get(const char *url) const;

    private:
        /// @brief sends a request with the given method and writes the
        ///        response data into a file (serialization into structs
        ///        will be added later)
        /// @param method HTTP method to use
        /// @param url target url
        /// @return status of the request
        response_status fetch_core(request_method method, const char *url) const;

        /// @brief
        /// @param str
        void print_info(const char *str) const;

        /// @brief
        /// @param str
        void print_error(const char *str) const;
    };
}
