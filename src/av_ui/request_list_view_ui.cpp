#include <av_ui/request_list_view_ui.hpp>

namespace avUi
{
    RequstListViewUi::RequstListViewUi(std::string id) : avR::UiComponent(std::move(id))
    {
    }

    RequstListViewUi::~RequstListViewUi()
    {
    }

    void RequstListViewUi::render()
    {
        ImGui::Begin(this->get_id().c_str());
        ImGui::End();
    }
} // namespace avUi
