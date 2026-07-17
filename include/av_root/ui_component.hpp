#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <misc/cpp/imgui_stdlib.h>

#include <GLFW/glfw3.h>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <av_root/root.hpp>

namespace avR
{
    /// @brief Current app-wide UI scale (tied to ImGui FontGlobalScale).
    inline float ui_scale() noexcept
    {
        return ImGui::GetIO().FontGlobalScale;
    }

    /// @brief Scale a pixel extent. Keeps 0 as 0 (ImGui "fill / auto" sentinel).
    inline float scale_px(float v) noexcept
    {
        return v == 0.0f ? 0.0f : v * ui_scale();
    }

    /// @brief Scale a size; 0 on either axis stays 0 (fill available).
    inline ImVec2 scale_size(ImVec2 v) noexcept
    {
        return ImVec2(scale_px(v.x), scale_px(v.y));
    }

    /// @brief Base of every UI element. Composite node: may hold children and/or
    ///        render itself. Rendered once per frame via Draw().
    class UiComponent
    {
    public:
        /// @brief Visual tunables. Defaults mirror the ImGui dark theme so an
        ///        unstyled button looks like before.
        struct Style
        {
            ImVec2 size = ImVec2(0.0f, 0.0f);     ///< (0,0) => auto-fit label
            ImVec2 padding = ImVec2(12.0f, 6.0f); ///< frame padding around label
            ImVec4 background = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            ImVec4 hovered = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            ImVec4 active = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
            ImVec4 text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            float rounding = 0.0f; ///< corner rounding in px
            float border = 0.0f;   ///< frame border thickness (0 = none)
        };

    public:
        UiComponent(const UiComponent &) = delete;
        UiComponent &operator=(const UiComponent &) = delete;
        virtual ~UiComponent() = default;

        explicit UiComponent(std::string id = {});

        /// @brief Per-frame entry point. Non-virtual on purpose: it opens this
        ///        node's unique ImGui ID scope (so identical sibling labels never
        ///        collide) and then dispatches to render(). Every node in the tree
        ///        is drawn through here, so the ID-scope invariant holds for all of
        ///        them without any component having to remember it. Subclasses
        ///        implement render(), never draw().
        void draw();

        /// @brief Layout negotiation used by resizable containers: a parent may
        ///        impose a size for the current frame (set_layout_size) and query
        ///        a child's preferred size. Leaf components ignore both.
        virtual void set_layout_size(const ImVec2 &size) { (void)size; }
        virtual ImVec2 preferred_size() const { return ImVec2(0.0f, 0.0f); }

        UiComponent *add_child(std::unique_ptr<UiComponent> child);
        UiComponent &set_on_click(std::function<void()> handler);
        const std::string &get_id() const noexcept;

    protected:
        /// @brief Emit this component's ImGui calls for the current frame. Invoked
        ///        by draw() inside this node's ID scope; use ScopedStyle for any
        ///        style pushes so they unwind automatically. This is the one method
        ///        a new component type must implement.
        virtual void render() = 0;

        /// @brief Logging for subclasses, forwarded to the composed AvRoot (tagged
        ///        with this component's id). AvRoot is held by composition, not
        ///        inherited — a UiComponent *has* a logger, it is not one.
        void log_info(std::string_view msg) const noexcept { this->root.log_info(msg); }
        void log_error(std::string_view msg) const noexcept { this->root.log_error(msg); }

        void fire_click() const;
        void draw_children();

        /// @brief Read access to the child list for layout components that need to
        ///        position children themselves (e.g. horizontal rows). Named
        ///        get_children() so the data member can simply be `children`.
        const std::vector<std::unique_ptr<UiComponent>> &get_children() const noexcept { return this->children; }

    private:
        // Declared before `id`: the ctor copies the id into root's name, then
        // moves it into `id` — member init runs in declaration order, so root
        // must be constructed while the id string is still intact.
        AvRoot root;
        std::string id;
        std::vector<std::unique_ptr<UiComponent>> children;
        std::function<void()> onClick;
    };
} // namespace avR
