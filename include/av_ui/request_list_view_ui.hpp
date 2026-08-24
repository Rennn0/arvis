#pragma once

#include <av_root/ui_component.hpp>
#include <av_root/av_request.hpp>
#include <av_root/av_inter_view_shared_state.hpp>
#include <av_root/av_request_list_state.hpp>
#include <av_ui/tab_bar_ui.hpp>
#include <av_s/av_request_storage.hpp>

namespace avUi
{
    class RequstListViewUi : public avR::UiComponent
    {
    public:
        explicit RequstListViewUi(std::string id);
        explicit RequstListViewUi(std::string id, avR::AvState *sharedState);
        ~RequstListViewUi();

    private:
        void render() override;
        void update() override;
        
        ImGuiWindowFlags windowFlags;
        avR::AvInterViewSharedState *shared_state;
        std::string filter_text;
        std::shared_ptr<avR::AvRequestListState> request_list_state;
        std::unique_ptr<avS::AvRequestStorage> request_storage;
        std::unique_ptr<avUi::TabBarUi> tabs;

        bool _tab_badge_enabled;
        int _tab_history_counter_badge = 0;
        int _tab_saved_counter_badge = 0;

        std::optional<int64_t> pending_delete_req;

        void render_header(const ImGuiStyle &style);
        void render_main_content(const ImGuiStyle &style);
        void render_footer(const ImGuiStyle &style);
        void render_tab_history(const ImGuiStyle &style);
        void render_tab_saved(const ImGuiStyle &style);
        void render_request_row(const avR::AvRequest *selected, avR::AvRequest *request, const ImGuiStyle &style);

        void new_request();
    };
} // namespace avUi
