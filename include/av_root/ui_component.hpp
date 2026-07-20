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
#include <av_root/ui_scoped_style.hpp>
#include <av_net/network_manager.hpp>

namespace avR
{
    /// @brief Base of every UI element. Composite node: may hold children and/or
    ///        render itself. Rendered once per frame via Draw().
    class UiComponent
    {

    public: // helpers
        const ImVec4 environment_color = ImColor(8, 249, 24);

        ImVec4 get_method_color(const avNet::request_method &method);
        ImVec4 get_status_color(const int code);

    public:
        UiComponent(const UiComponent &) = delete;
        UiComponent &operator=(const UiComponent &) = delete;
        virtual ~UiComponent() = default;

        explicit UiComponent(std::string id);

        /// @brief Per-frame entry point. Non-virtual on purpose: it opens this
        ///        node's unique ImGui ID scope (so identical sibling labels never
        ///        collide) and then dispatches to render(). Every node in the tree
        ///        is drawn through here, so the ID-scope invariant holds for all of
        ///        them without any component having to remember it. Subclasses
        ///        implement render(), never draw().
        virtual void draw();

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

        /// @brief component specific style per frame
        avR::UiScopedStyle::Style style;

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
