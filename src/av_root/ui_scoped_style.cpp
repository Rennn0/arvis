#include <av_root/ui_scoped_style.hpp>
#include <av_root/ui_component.hpp>

namespace avR
{

    UiScopedStyle::UiScopedStyle(const Style &style) : style_count(0), color_count(0)
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
        push(ImGuiStyleVar_FrameBorderSize, style.frame_border);
    }

    UiScopedStyle::UiScopedStyle() : style_count(0), color_count(0)
    {
    }

    UiScopedStyle::~UiScopedStyle()
    {
        // separate stacks: colours and vars each pop their own count
        if (this->color_count > 0)
        {
            ImGui::PopStyleColor(static_cast<int>(this->color_count));
        }

        if (this->style_count > 0)
        {
            ImGui::PopStyleVar(static_cast<int>(this->style_count));
        }
    }

    UiScopedStyle &UiScopedStyle::color(ImGuiCol idx, const ImVec4 &value)
    {
        ImGui::PushStyleColor(idx, value);
        ++this->color_count;
        return *this;
    }

    UiScopedStyle &UiScopedStyle::color(ImGuiCol idx, ImU32 value)
    {
        ImGui::PushStyleColor(idx, value);
        ++this->color_count;
        return *this;
    }

    UiScopedStyle &UiScopedStyle::var(ImGuiStyleVar idx, float value)
    {
        ImGui::PushStyleVar(idx, value);
        ++this->style_count;
        return *this;
    }

    UiScopedStyle &UiScopedStyle::var(ImGuiStyleVar idx, const ImVec2 &value)
    {
        ImGui::PushStyleVar(idx, value);
        ++this->style_count;
        return *this;
    }

} // namespace avR
