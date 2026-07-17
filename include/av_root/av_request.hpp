#pragma once

#include <string>
#include <av_net/network_manager.hpp>

namespace avR
{
    /// @brief One user-created request entry (the app-side model the UI lists,
    ///        selects and displays; sending it is wired up later).
    struct AvRequest
    {
        int id = 0;                                                ///< sequential, used for display name
        std::string url = "https://";                              ///< target url
        avNet::request_method method = avNet::request_method::get; ///< HTTP method
        std::string body;                                          ///< payload (POST)
        std::string title;                                         ///< user-editable title
        bool hasTitle = !title.empty();                            ///< whether the user has set a title
        /// @brief name shown in the sidebar list, e.g. "req#3"
        std::string display_name() const { return "req#" + std::to_string(id); }
    };
} // namespace avR
