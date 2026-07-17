#include <av_ui/detailed_request_view_ui.hpp>
#include <av_ui/request_list_ui.hpp>


namespace avUi
{
    DetailedRequestViewUi::DetailedRequestViewUi(std::shared_ptr<avR::AvState> statePtr, std::string id)
        : avR::UiStatefullComponent(statePtr, std::move(id))
    {
    }

    DetailedRequestViewUi::~DetailedRequestViewUi()
    {
    }

    void DetailedRequestViewUi::render()
    {
        RequstListViewUi::RequestListState *state = static_cast<RequstListViewUi::RequestListState *>(get_state());
        if (state->selected_request_index < 0 || state->selected_request_index >= state->requests.size())
        {
            ImGui::TextDisabled("select a request from the sidebar");
            return;
        }
        avR::AvRequest &req = state->requests[state->selected_request_index];
        ImGui::InputText("##display_name", /*&req.display_name()*/ &req.title);
        ImGui::Separator();
        ImGui::Text("id:     %d", req.id);

        const avNet::request_method kMethods[] = {
            avNet::request_method::get,
            avNet::request_method::post,
            avNet::request_method::put,
            avNet::request_method::patch,
            avNet::request_method::del,
            avNet::request_method::head,
            avNet::request_method::options,
        };
        ImGui::Text("method:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.f);
        if (ImGui::BeginCombo("##method", avNet::NetworkManager::method_text(req.method)))
        {
            for (size_t i = 0; i < sizeof(kMethods);i++)
            {
                bool selected = req.method == kMethods[i];
                if (ImGui::Selectable(avNet::NetworkManager::method_text(kMethods[i]), selected))
                {
                    req.method = kMethods[i];
                }

                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Text("url:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(400.f);
        ImGui::InputText("##url", &req.url);


        ImGui::Text("body:");
        ImGui::InputTextMultiline("##body", &req.body, ImVec2(400, ImGui::GetTextLineHeight() * 4));
    }
} // namespace avUi
