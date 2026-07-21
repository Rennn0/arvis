#pragma once
#include <av_root/av_request.hpp>
#include <av_root/ui_component.hpp>
#include <av_root/av_inter_view_shared_state.hpp>
#include <av_s/av_request_storage.hpp>

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

        ImGuiWindowFlags window_flags;
        avR::AvInterViewSharedState *shared_state;
        std::unique_ptr<avS::AvRequestStorage> request_storage;

        void render_header(const ImGuiStyle &style);
        void render_main_content(const ImGuiStyle &style);
        void render_footer(const ImGuiStyle &style);

        void save_state_change() const;
    };
} // namespace avUi
