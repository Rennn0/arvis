#include <av_root/ui_component.hpp>

namespace avR
{
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
