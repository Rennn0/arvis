#pragma once
#include <cstdint>
#include <string>
#include <chrono>
#include <cctype>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <av_root/root.hpp>

namespace avNet
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

    /// @brief full outcome of a request: status plus the fetched body and,
    ///        on success, the path the body was saved to.
    struct http_result
    {
        response_status status = response_status::Failed;
        long http_code = 0;     ///< HTTP status code (0 if the request never completed)
        std::string body;       ///< raw response body
        std::string saved_path; ///< file the body was written to (empty on failure)
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
        http_result get(const char *url) const;

        /// @brief sends post request and writes response data into folder
        /// @param url
        http_result post(const char *url) const;

    private:
        avR::AvRoot m_root;        

        /// @brief sends a request with the given method, writes the response
        ///        body into the responses folder, and returns the full result
        ///        (status, HTTP code, body, saved path) for display.
        /// @param method HTTP method to use
        /// @param url target url
        /// @return full result of the request
        http_result fetch_core(request_method method, const char *url) const;
    };
}
