#pragma once

#include <functional>
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
        AvRequest *display_request;
        avUi::UiShortcut shortcut;

        std::function<void()> on_new_request;

    public:
        AvInterViewSharedState();
        ~AvInterViewSharedState();

    private:
    };
} // namespace avR
