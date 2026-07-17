#include <av_root/av_button.hpp>
#include <av_root/im_scope.hpp>

namespace avR
{
    AvButton::AvButton(std::string label, Style style)
        : UiComponent(label), label(std::move(label)), style(style)
    {
    }

    AvButton::~AvButton()
    {
        this->label.clear();
    }

    ImVec2 AvButton::preferred_size() const
    {
        if (this->style.size.x > 0.0f || this->style.size.y > 0.0f)
            return scale_size(this->style.size);

        // Auto-sized: approximate with label width + frame padding (matches draw()).
        const ImVec2 pad = scale_size(this->style.padding);
        const ImVec2 text = ImGui::CalcTextSize(this->label.c_str());
        return ImVec2(text.x + pad.x * 2.0f, text.y + pad.y * 2.0f);
    }

    void AvButton::render()
    {
        ScopedStyle styleScope;
        styleScope.color(ImGuiCol_Button, this->style.background)
            .color(ImGuiCol_ButtonHovered, this->style.hovered)
            .color(ImGuiCol_ButtonActive, this->style.active)
            .color(ImGuiCol_Text, this->style.text)
            .var(ImGuiStyleVar_FrameRounding, scale_px(this->style.rounding))
            .var(ImGuiStyleVar_FramePadding, scale_size(this->style.padding))
            .var(ImGuiStyleVar_FrameBorderSize, this->style.border);

        if (ImGui::Button(this->label.c_str(), scale_size(this->style.size)))
        {
            log_info("clicked");
            fire_click();
        }
    }
} // namespace avR
