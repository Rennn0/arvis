#pragma once
#include <av_root/ui_component.hpp>

namespace avR
{
    class UiContainerComponent : public UiComponent
    {
    public:
        /// @brief Axis children are laid out along.
        enum class Direction : uint8_t
        {
            Vertical,   ///< top-to-bottom (default)
            Horizontal, ///< left-to-right
        };

        /// @brief All tunables in one place so the container is configurable
        ///        without a wide constructor.
        struct Config
        {
            Direction direction = Direction::Vertical;             ///< stacking axis
            bool border = false;                                   ///< draw a border around the region
            ImVec2 size = ImVec2(0.0f, 0.0f);                      ///< (0,0) => fill available space
            ImVec2 padding = ImVec2(8.0f, 8.0f);                   ///< inner padding
            float spacing = 6.0f;                                  ///< gap between children
            ImVec4 background = ImVec4(0.03f, 0.05f, 0.10f, 1.0f); ///< alpha 0 => transparent

            // --- resizable splitters -------------------------------------------
            // When enabled, a draggable divider is placed on the trailing edge of
            // every child except the last; dragging it resizes that child along
            // the main axis (left/right for Horizontal, up/down for Vertical).
            bool resizable = false;          ///< place draggable dividers between children
            float splitter_thickness = 6.0f; ///< divider hit/draw thickness in px
            float resize_min = 40.0f;        ///< min extent of a resized child
            float resize_max = 0.0f;         ///< max extent (0 => unlimited)
        };
        // // Navy
        // ImVec4(0.05f, 0.08f, 0.18f, 1.0f);

        // // Slate blue
        // ImVec4(0.10f, 0.14f, 0.24f, 1.0f);

        // // Steel blue (slightly brighter)
        // ImVec4(0.15f, 0.22f, 0.35f, 1.0f);

        // // Very dark blue
        // ImVec4(0.03f, 0.05f, 0.10f, 1.0f);
    public:
        explicit UiContainerComponent(std::string label);
        UiContainerComponent() = delete;
        ~UiContainerComponent();

    private:
        std::string label;
    };
} // namespace avR
