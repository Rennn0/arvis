#include "network_manager.hpp"
#include <chrono>
#include <cctype>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace
{

    constexpr const char *RESPONSES_DIR = "responses";

    std::string sanitize_for_filename(const char *url)
    {
        std::string result;
        for (const char *p = url; *p; ++p)
        {
            const unsigned char c = static_cast<unsigned char>(*p);
            if (std::isalnum(c))
            {
                result += static_cast<char>(c);
            }
            else
            {
                result += '_';
            }
        }
        return result;
    }

    std::string timestamp_string()
    {
        const auto now = std::chrono::system_clock::now();
        const std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&t, &tm);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
        return oss.str();
    }

    size_t write_callback(char *data, size_t size, size_t nmemb, void *userp)
    {
        auto *response = static_cast<std::string *>(userp);
        response->append(data, size * nmemb);
        return size * nmemb;
    }

}

NetworkManager::NetworkManager()
{
    print_info("network manager ctr");
    CURLcode initCode = curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string msg = "curl_global_init status: ";
    msg += curl_easy_strerror(initCode);
    print_info(msg.c_str());
}

NetworkManager::~NetworkManager()
{
    print_info("network manager dectr");
    curl_global_cleanup();
}

void NetworkManager::get(const char *url) const
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        print_info("curl_easy_init failed");
        return;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        print_error(curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return;
    }

    const std::string filename = sanitize_for_filename(url) + timestamp_string() + ".txt";
    const std::filesystem::path filepath = std::filesystem::path(RESPONSES_DIR) / filename;

    std::error_code ec;
    std::filesystem::create_directories(RESPONSES_DIR, ec);
    if (ec) {
        print_error("failed to create responses directory");
        curl_easy_cleanup(curl);
        return;
    }

    std::ofstream out(filepath, std::ios::binary);
    if (!out)
    {
        print_error("failed to open output file");
        curl_easy_cleanup(curl);
        return;
    }

    out.write(response.data(), static_cast<std::streamsize>(response.size()));
    if (!out)
    {
        print_error("failed to write output file");
    }
    else
    {
        std::string msg = "saved response to ";
        msg += filepath.string();
        print_info(msg.c_str());
    }

    curl_easy_cleanup(curl);
}

void NetworkManager::print_info(const char *str) const
{
    std::cout << "[info] " << str << std::endl;
}

void NetworkManager::print_error(const char *str) const
{
    std::cout << "[error] " << str << std::endl;
}
