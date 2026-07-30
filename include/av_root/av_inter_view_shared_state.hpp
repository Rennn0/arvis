#pragma once

#include <functional>
#include <optional>
#include <av_root/av_state.hpp>
#include <av_root/av_request.hpp>
#include <av_ui/ui_shortcut.hpp>

namespace avR
{
    class AvInterViewSharedState : public AvState
    {
    public:
        bool show_req_list_view;
        bool show_req_detailed_view;
        bool show_search_view = false;
        bool is_init = false;
        AvRequest *display_request;
        std::optional<std::function<void()>> on_display_request_change;

        // key binding actions //////////////////////////////////
        avUi::UiShortcut shortcutManager;
        std::optional<std::function<void()>> on_new_request;
        std::optional<std::function<void()>> on_save_changes;
        std::optional<std::function<void()>> on_send_request;
        std::optional<std::function<void()>> on_rename_request;
        std::optional<std::function<void()>> on_show_shortcuts;
        std::optional<std::function<void()>> on_show_style_editor;
        //////////////////////////////////////////////////////////
    public:
        AvInterViewSharedState();
        ~AvInterViewSharedState();

    private:
    };
} // namespace avR
