#pragma once
#include <optional>
#include <imgui.h>

namespace avR
{
    /// @brief RAII guard over ImGui's style stacks. Two ways in, and they compose:
    ///        construct from a Style block for the fixed set of window/frame vars,
    ///        and/or chain color()/var() for one-off pushes. Everything pushed is
    ///        popped in the destructor, so pop counts are never hand-written.
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
            std::optional<float> frame_border;
        };

    public:
        explicit UiScopedStyle(const Style &style);

        /// @brief Pushes nothing. Chain color()/var() onto the named local to fill it.
        UiScopedStyle();
        ~UiScopedStyle();

        // a guard owns its pop counts; copying would double-pop
        UiScopedStyle(const UiScopedStyle &) = delete;
        UiScopedStyle &operator=(const UiScopedStyle &) = delete;

        /// @brief Push one ImGuiCol_ override, popped with the guard. Chainable.
        UiScopedStyle &color(ImGuiCol idx, const ImVec4 &value);
        UiScopedStyle &color(ImGuiCol idx, ImU32 value);

        /// @brief Push one ImGuiStyleVar_ override that Style does not cover. Chainable.
        UiScopedStyle &var(ImGuiStyleVar idx, float value);
        UiScopedStyle &var(ImGuiStyleVar idx, const ImVec2 &value);

    private:
        size_t style_count;
        size_t color_count;
    };
} // namespace avR
