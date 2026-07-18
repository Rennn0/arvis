#include <av_ui/request_list_view_ui.hpp>

namespace avUi
{
    RequstListViewUi::RequstListViewUi(std::string id) : avR::UiComponent(std::move(id))
    {
        this->windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove;
    }

    RequstListViewUi::RequstListViewUi(std::string id, avR::AvState *sharedState) : RequstListViewUi(id)
    {
        this->shared_state = static_cast<avR::AvInterViewSharedState *>(sharedState);
    }

    RequstListViewUi::~RequstListViewUi()
    {
    }

    void RequstListViewUi::render()
    {
        if (this->shared_state && !this->shared_state->show_req_list_view)
            return;

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Always);
        if (ImGui::Begin(this->get_id().c_str(), &this->shared_state->show_req_list_view, this->windowFlags))
        {
        };
        ImGui::End();
    }
} // namespace avUi
