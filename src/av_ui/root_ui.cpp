#include <av_ui/root_ui.hpp>
#include <av_ui/request_list_view_ui.hpp>
#include <av_ui/detailed_request_view_ui.hpp>
#include <av_root/av_inter_view_shared_state.hpp>

namespace avUi
{
    RootUi::RootUi(std::string id)
        : UiComponent(id), viewport(ImGui::GetMainViewport()),
          inter_view_state(std::make_shared<avR::AvInterViewSharedState>())
    {
        ImGuiStyle &style = ImGui::GetStyle();
        style.WindowRounding = 5.f;
        style.WindowBorderSize = 0.f;

        this->add_child(std::make_unique<avUi::RequstListViewUi>("req_list_view", this->inter_view_state.get()));
        this->add_child(std::make_unique<avUi::DetailedRequestViewUi>("detailed_view", this->inter_view_state.get()));
    }

    RootUi::~RootUi()
    {
        viewport = nullptr;
    }

    void RootUi::render()
    {
        for (const std::unique_ptr<UiComponent> &child : get_children())
            child->draw();
    }
} // namespace avUi
