#include <av_root/ui_component.hpp>

namespace avR
{
    ImVec4 UiComponent::get_method_color(const avNet::request_method &method)
    {
        switch (method)
        {
        case avNet::request_method::get:
            return ImVec4(0.40f, 0.80f, 0.40f, 1.0f); // green
        case avNet::request_method::post:
            return ImVec4(0.90f, 0.70f, 0.30f, 1.0f); // amber
        case avNet::request_method::put:
            return ImVec4(0.40f, 0.60f, 0.90f, 1.0f); // blue
        case avNet::request_method::patch:
            return ImVec4(0.35f, 0.80f, 0.78f, 1.0f); // teal
        case avNet::request_method::del:
            return ImVec4(0.90f, 0.40f, 0.40f, 1.0f); // red
        case avNet::request_method::head:
            return ImVec4(0.72f, 0.55f, 0.95f, 1.0f); // violet
        case avNet::request_method::options:
            return ImVec4(0.90f, 0.55f, 0.72f, 1.0f); // rose
        }
        return ImVec4(0.70f, 0.70f, 0.70f, 1.0f); // fallback
    }

    ImVec4 UiComponent::get_status_color(const int code)
    {
        if (code >= 200 && code < 300)
            return ImVec4(0.40f, 0.80f, 0.40f, 1.0f); // 2xx success - green
        if (code >= 300 && code < 400)
            return ImVec4(0.40f, 0.60f, 0.90f, 1.0f); // 3xx redirect - blue
        if (code >= 400)
            return ImVec4(0.90f, 0.40f, 0.40f, 1.0f); // 4xx/5xx error - red
        return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);     // 1xx / no response - gray
    }

    UiComponent::UiComponent(std::string id) : style(avR::UiScopedStyle::Style{}), root(id), id(id)
    {
    }

    void UiComponent::draw()
    {
        ImGui::PushID(this->id.c_str());
        avR::UiScopedStyle scoped(this->style);
        render();
        ImGui::PopID();
    }

    UiComponent *UiComponent::add_child(std::unique_ptr<UiComponent> child)
    {
        this->children.push_back(std::move(child));
        return this->children.back().get();
    }

    UiComponent &UiComponent::set_on_click(std::function<void()> handler)
    {
        this->onClick = std::move(handler);
        return *this;
    }

    const std::string &UiComponent::get_id() const noexcept
    {
        return this->id;
    }

    void UiComponent::fire_click() const
    {
        if (this->onClick)
            this->onClick();
    }

    void UiComponent::draw_children()
    {
        for (const std::unique_ptr<UiComponent> &child : this->children)
            child->draw();
    }
} // namespace avR
