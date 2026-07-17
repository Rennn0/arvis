#include <av_root/ui_scoped_style.hpp>
#include <av_root/ui_component.hpp>

namespace avR
{

    UiScopedStyle::UiScopedStyle(const Style &style) : style_count(0)
    {
        if (style.window_rounding.has_value())
        {
            style_count++;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, *style.window_rounding);
        }

        if (style.window_border_size.has_value())
        {
            style_count++;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, *style.window_border_size);
        }
    }

    UiScopedStyle::~UiScopedStyle()
    {
        if (this->style_count > 0)
        {
            ImGui::PopStyleVar(this->style_count);
        }
    }

} // namespace avR