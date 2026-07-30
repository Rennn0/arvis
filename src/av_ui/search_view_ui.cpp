#include <av_ui/search_view_ui.hpp>

namespace avUi
{
    SearchViewUi::~SearchViewUi()
    {
    }
    SearchViewUi::SearchViewUi(std::string id, avR::AvState *sharedState) : UiComponent(id)
    {
    }
    void SearchViewUi::render()
    {
        if (ImGui::Begin("search"))
        {
        }
        ImGui::End();
    }
} // namespace avUi
