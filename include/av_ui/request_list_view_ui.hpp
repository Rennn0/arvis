#pragma once

#include <av_root/ui_component.hpp>
#include <av_root/av_request.hpp>
#include <av_root/av_state.hpp>

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
        ~RequstListViewUi();

    private:
        void render() override;
    };
} // namespace avUi
