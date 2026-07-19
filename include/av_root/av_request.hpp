#pragma once

#include <optional>
#include <string>
#include <av_net/network_manager.hpp>

namespace avR
{
    /// @brief One user-created request entry (the app-side model the UI lists,
    ///        selects and displays; sending it is wired up later).
    struct AvRequest
    {
        int64_t id = 0;
        int64_t timestamp = 0;
        avNet::request_method method = avNet::request_method::get;
        std::string url = "https://";
        std::optional<std::string> body;
        std::optional<std::string> title;
        const std::string display_name() const { return title.value_or("request#" + std::to_string(id)); }
    };
} // namespace avR
