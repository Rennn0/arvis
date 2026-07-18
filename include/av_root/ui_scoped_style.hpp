#pragma once
#include <optional>
#include <imgui.h>

namespace avR
{
    class UiScopedStyle
    {
    public:
        struct Style
        {
            std::optional<float> window_rounding;
            std::optional<float> window_border_size;
            std::optional<ImVec2> window_padding;
            std::optional<float> frame_rounding;
            std::optional<ImVec2> frame_padding;
        };

    public:
        explicit UiScopedStyle(const Style &style);
        UiScopedStyle() = delete;
        ~UiScopedStyle();

    private:
        size_t style_count;
    };
} // namespace avR
