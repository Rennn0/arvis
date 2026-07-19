#pragma once
#include <av_root/av_request.hpp>
#include <av_root/ui_component.hpp>
#include <av_root/av_inter_view_shared_state.hpp>

namespace avUi
{
    class DetailedRequestViewUi : public avR::UiComponent
    {
    public:
        explicit DetailedRequestViewUi(std::string id);
        explicit DetailedRequestViewUi(std::string id, avR::AvState *sharedState);
        ~DetailedRequestViewUi();

    private:
        float footer_height;

        void render() override;

        ImGuiWindowFlags windowFlags;
        avR::AvInterViewSharedState *shared_state;

        void render_header(const ImGuiStyle &style);
        void render_main_content(const ImGuiStyle &style);
        void render_footer(const ImGuiStyle &style);
    };
} // namespace avUi
