#include <av_ui/detailed_request_view_ui.hpp>

namespace avUi
{
    DetailedRequestViewUi::DetailedRequestViewUi(std::string id) : avR::UiComponent(std::move(id))
    {
        this->windowFlags = ImGuiWindowFlags_NoCollapse;
    }

    DetailedRequestViewUi::DetailedRequestViewUi(std::string id, avR::AvState *sharedState) : DetailedRequestViewUi(id)
    {
        this->shared_state = static_cast<avR::AvInterViewSharedState *>(sharedState);
    }

    DetailedRequestViewUi::~DetailedRequestViewUi()
    {
    }

    void DetailedRequestViewUi::render()
    {
        if (this->shared_state && !this->shared_state->show_req_detailed_view)
            return;

        if (ImGui::Begin(this->get_id().c_str(), &this->shared_state->show_req_detailed_view, this->windowFlags))
        {
            ImGui::Checkbox("x", &this->shared_state->show_req_list_view);

            ImGui::ShowStyleEditor();
        }
        ImGui::End();
    }
} // namespace avUi
