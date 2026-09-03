#pragma once

#include <functional>
#include <optional>
#include <av_root/av_state.hpp>
#include <av_root/av_request.hpp>
#include <av_ui/ui_shortcut.hpp>
#include <av_root/av_request_list_state.hpp>
#include <av_root/av_app_settings.hpp>

namespace avR
{
    class AvInterViewSharedState : public AvState
    {
    public:
        bool show_req_list_view;
        bool show_req_detailed_view;
        bool show_search_view = false;
        bool show_settings_view = false;
        bool is_init = false;

        const float _left_panel_ratio = .2f;
        const float _min_left_panel_width = 180.f;

        /// monotonic source of AvRequest::snapshot_id. Never reset, so a snapshot id is
        /// unique for the lifetime of the process. (Was recent_req_counter, which doubled
        /// as the history badge count and so went stale on every removal - the badge is
        /// now derived from the actual snapshot vectors.)
        int64_t recent_req_seq = 0;
        /// how many snapshots one saved request keeps before the oldest is dropped
        size_t recent_req_limit = 25;

        AvRequest *display_request;
        /// Keeps a selected history snapshot alive while `display_request` points at it.
        /// Snapshots live in AvRequest::recent_reqs, which history trimming, "clear" and
        /// deleting the origin all shrink underneath the raw pointer. Empty whenever a
        /// saved request is selected - those are owned by AvRequestListState::requests.
        std::shared_ptr<AvRequest> display_request_hold;
        avR::AvRequestListState *request_list_state;
        std::shared_ptr<avR::AvAppSettings> app_settings;
        std::optional<std::function<void()>> on_display_request_change;
        std::optional<std::function<void()>> on_env_change;

        // key binding actions //////////////////////////////////
        avUi::UiShortcut shortcutManager;
        std::optional<std::function<void()>> on_new_request;
        std::optional<std::function<void()>> on_save_changes;
        std::optional<std::function<void()>> on_send_request;
        std::optional<std::function<void()>> on_rename_request;
        std::optional<std::function<void()>> on_show_shortcuts;
        std::optional<std::function<void()>> on_show_style_editor;
        std::optional<std::function<void()>> on_show_search;
        std::optional<std::function<void(size_t)>> on_show_settings;
        //////////////////////////////////////////////////////////
    public:
        AvInterViewSharedState();
        ~AvInterViewSharedState();

    private:
    };
} // namespace avR
