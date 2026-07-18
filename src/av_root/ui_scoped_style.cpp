#include <av_root/ui_scoped_style.hpp>
#include <av_root/ui_component.hpp>

namespace avR
{

    UiScopedStyle::UiScopedStyle(const Style &style) : style_count(0)
    {
        auto push = [this](ImGuiStyleVar idx, const auto &opt)
        {
            if (opt.has_value())
            {
                ImGui::PushStyleVar(idx, *opt);
                ++this->style_count;
            }
        };

        push(ImGuiStyleVar_WindowRounding, style.window_rounding);
        push(ImGuiStyleVar_WindowPadding, style.window_padding);
        push(ImGuiStyleVar_WindowBorderSize, style.window_border_size);
        push(ImGuiStyleVar_FrameRounding, style.frame_rounding);
        push(ImGuiStyleVar_FramePadding, style.frame_padding);
    }

    UiScopedStyle::~UiScopedStyle()
    {
        if (this->style_count > 0)
        {
            ImGui::PopStyleVar(this->style_count);
        }
    }

} // namespace avR