#include <av_ui/detailed_request_view_ui.hpp>

namespace avUi
{
    DetailedRequestViewUi::DetailedRequestViewUi(std::string id) : avR::UiComponent(std::move(id))
    {
    }

    DetailedRequestViewUi::~DetailedRequestViewUi()
    {
    }

    void DetailedRequestViewUi::render()
    {
        ImGui::Begin(this->get_id().c_str());
        ImGui::End();
    }
} // namespace avUi
