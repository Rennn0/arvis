#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>
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
        put,
        patch,
        del,
        head,
        options
    };

    /// @brief full outcome of a request: status plus the fetched body and,
    ///        on success, the path the body was saved to.
    struct http_result
    {
        response_status status = response_status::Failed;
        long http_code = 0;     ///< HTTP status code (0 if the request never completed)
        std::string body;       ///< raw response body
        std::string saved_path; ///< file the body was written to (empty on failure)
        std::vector<std::pair<std::string, std::string>> response_headers;
        std::vector<std::pair<std::string, std::string>> response_cookies;
        long long elapsed_mc = 0;
    };
    /// @brief self-contained description of a request to send. It owns its own
    ///        strings so a worker thread can keep a snapshot alive independently
    ///        of any UI-side model that produced it.
    struct http_request
    {
        request_method method = request_method::get;
        std::string url;
        /// header lines as (key, value) pairs; empty keys are skipped when sending.
        std::vector<std::pair<std::string, std::string>> headers;
        /// cookies as (name, value) pairs; joined into one `Cookie:` line when sending.
        /// Empty names are skipped.
        std::vector<std::pair<std::string, std::string>> cookies;
        /// request body; sent for methods that carry one (POST/PUT/PATCH/DELETE).
        std::optional<std::string> body;
    };

    /// @brief abstractions over some http methods, uses libcurl
    class NetworkManager
    {
    public:
        static const char *status_text(avNet::response_status s)
        {
            switch (s)
            {
            case avNet::response_status::Ok:
                return "Ok";
            case avNet::response_status::Failed:
                return "Failed";
            case avNet::response_status::Canceled:
                return "Canceled";
            }
            return "Unknown";
        }

        static const char *method_text(avNet::request_method m)
        {
            switch (m)
            {
            case avNet::request_method::get:
                return "GET";
            case avNet::request_method::post:
                return "POST";
            case avNet::request_method::put:
                return "Put";
            case avNet::request_method::patch:
                return "Patch";
            case avNet::request_method::head:
                return "Head";
            case avNet::request_method::options:
                return "Options";
            case avNet::request_method::del:
                return "Delete";
            }
            return "Unknown";
        }

    public:
        /// @brief global init
        NetworkManager(std::string_view path);

        /// @brief global cleanup
        ~NetworkManager();

        /// @brief sends get request and writes response data into folder
        /// @param url
        http_result get(const char *url) const;

        /// @brief sends a get request and writes the response body into @p out_body.
        ///        The body is still saved to the responses folder like the
        ///        single-argument overload.
        /// @param url target url
        /// @param out_body buffer that receives the response body (or the error text on failure)
        /// @param out_http_code optional out-param that receives the HTTP status code
        /// @return status of the request
        response_status get(const char *url, std::string &out_body, long *out_http_code = nullptr) const;

        /// @brief sends post request and writes response data into folder
        /// @param url
        http_result post(const char *url) const;

        /// @brief sends a fully-assembled request (method + url + headers + body),
        ///        writes the response body into @p out_body, and saves it to the
        ///        responses folder like the other overloads.
        /// @param request the request to send
        /// @param out_body buffer that receives the response body (or the error text on failure)
        /// @param out_http_code optional out-param that receives the HTTP status code
        /// @return status of the request
        response_status send(const http_request &request, std::string &out_body, long *out_http_code = nullptr) const;

        http_result send(const http_request &request) const;

    private:
        avR::AvRoot root;
        std::filesystem::path save_path;
        /// @brief sends the given request, writes the response body into the
        ///        responses folder, and returns the full result (status, HTTP
        ///        code, body, saved path) for display.
        /// @param request the request to send
        /// @return full result of the request
        http_result fetch_core(const http_request &request) const;
    };
} // namespace avNet
