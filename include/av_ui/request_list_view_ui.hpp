#pragma once

#include <av_root/ui_component.hpp>
#include <av_root/av_request.hpp>
#include <av_root/av_inter_view_shared_state.hpp>
#include <av_root/av_request_list_state.hpp>

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
        ImGuiWindowFlags windowFlags;
        avR::AvInterViewSharedState *shared_state;
        std::string filter_text;
        std::shared_ptr<avR::AvRequestListState> request_list_state;

        void render_header(const ImGuiStyle &style);
        void render_main_content(const ImGuiStyle &style);
        void render_footer(const ImGuiStyle &style);
    };
} // namespace avUi
