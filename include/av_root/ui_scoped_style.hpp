#pragma once
#include <optional>

namespace avR
{
    class UiScopedStyle
    {
    public:
        struct Style
        {
            std::optional<float> window_rounding;
            std::optional<float> window_border_size;
        };

    public:
        explicit UiScopedStyle(const Style &style);
        UiScopedStyle() = delete;
        ~UiScopedStyle();

    private:
        size_t style_count;
    };
} // namespace avR
