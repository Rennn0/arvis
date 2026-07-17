#include <av_ui/request_list_ui.hpp>

namespace avUi
{
    RequstListViewUi::RequstListViewUi(std::shared_ptr<avR::AvState> statePtr, std::string id)
        : UiStatefullComponent(statePtr, id)
    {
    }

    RequstListViewUi::~RequstListViewUi()
    {
    }
    
    void RequstListViewUi::render()
    {
        RequestListState* state = static_cast<RequestListState*>(get_state());
        ImGui::TextDisabled("requests");
        ImGui::Separator();
        for (size_t i = 0; i < state->requests.size();i++)
        {
            //ImGui::PushID(i);
            const auto &req = state->requests[i];
            const std::string label = req.hasTitle ?req.title : req.display_name();
            if (ImGui::Selectable(label.c_str(), state->selected_request_index == i))
            {
                state->selected_request_index = i;
            }
            //ImGui::PopID();
            if (state->requests.empty())
            {
                ImGui::TextDisabled("(empty — New request)");
            }
        }
    }
} // namespace avUi
