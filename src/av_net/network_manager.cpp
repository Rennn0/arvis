#include <av_net/network_manager.hpp>

namespace
{   
    constexpr const char *RESPONSES_DIR = "responses.dev";

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
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
        return oss.str();
    }

    size_t write_callback(char *data, size_t size, size_t nmemb, void *userp)
    {
        std::string *response = static_cast<std::string *>(userp);
        response->append(data, size * nmemb);
        return size * nmemb;
    }

    void apply_method(CURL *curl, avNet::request_method method)
    {
        switch (method)
        {
        case avNet::request_method::get:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
        case avNet::request_method::post:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            // Send an explicit (empty) body. Without this, CURLOPT_POST makes
            // libcurl read the request body from its default source (stdin),
            // which blocks forever in a GUI app that has no console input.
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
            break;
        }
    }

}

avNet::NetworkManager::NetworkManager() : root("NetworkManager")
{
    this->root.log_info("network manager ctr");
    CURLcode initCode = curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string msg = "curl_global_init status: ";
    msg += curl_easy_strerror(initCode);
    this->root.log_info(msg.c_str());
}

avNet::NetworkManager::~NetworkManager()
{
    this->root.log_info("network manager dectr");
    curl_global_cleanup();
}

avNet::http_result avNet::NetworkManager::get(const char *url) const
{
    return fetch_core(request_method::get, url);
}

avNet::http_result avNet::NetworkManager::post(const char *url) const
{
    return fetch_core(request_method::post, url);
}

avNet::http_result avNet::NetworkManager::fetch_core(request_method method, const char *url) const
{
    http_result result;

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        this->root.log_error("curl_easy_init failed");
        return result;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    apply_method(curl, method);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // Never let a request hang the worker thread indefinitely.
    // NOSIGNAL is required for timeouts to work safely off the main thread.
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        this->root.log_error(curl_easy_strerror(res));
        result.body = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        return result;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.http_code);
    result.body = response;

    const std::string filename = sanitize_for_filename(url) + timestamp_string() + ".txt";
    const std::filesystem::path filepath = std::filesystem::path(RESPONSES_DIR) / filename;

    std::error_code ec;
    std::filesystem::create_directories(RESPONSES_DIR, ec);
    if (ec)
    {
        this->root.log_error("failed to create responses directory");
        curl_easy_cleanup(curl);
        return result;
    }

    std::ofstream out(filepath, std::ios::binary);
    if (!out)
    {
        this->root.log_error("failed to open output file");
        curl_easy_cleanup(curl);
        return result;
    }

    out.write(response.data(), static_cast<std::streamsize>(response.size()));
    if (!out)
    {
        this->root.log_error("failed to write output file");
        curl_easy_cleanup(curl);
        return result;
    }

    std::string msg = "saved response to ";
    msg += filepath.string();
    this->root.log_info(msg.c_str());

    result.saved_path = filepath.string();
    result.status = response_status::Ok;

    curl_easy_cleanup(curl);
    return result;
}