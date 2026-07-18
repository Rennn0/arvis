#pragma once

#include <av_root/ui_component.hpp>
#include <av_root/av_request.hpp>
#include <av_root/av_inter_view_shared_state.hpp>

namespace avUi
{
    class RequstListViewUi : public avR::UiComponent
    {
    public:
        struct RequestListState : avR::AvState
        {
            int selected_request_index = -1;
            int next_request_id = 1;
            std::string display_name;
            std::vector<avR::AvRequest> requests;
        };

    public:
        explicit RequstListViewUi(std::string id);
        explicit RequstListViewUi(std::string id, avR::AvState *sharedState);
        ~RequstListViewUi();

    private:
        void render() override;
        ImGuiWindowFlags windowFlags;
        avR::AvInterViewSharedState *shared_state;
    };
} // namespace avUi
