#include <av_root/ui_scoped_id.hpp>

namespace avR
{
    UiScopedId::UiScopedId(const char *id)
    {
        ImGui::PushID(id);
    }

    UiScopedId::UiScopedId(int id)
    {
        ImGui::PushID(id);
    }

    UiScopedId::UiScopedId(const void *id)
    {
        ImGui::PushID(id);
    }

    UiScopedId::~UiScopedId()
    {
        ImGui::PopID();
    }
} // namespace avR
