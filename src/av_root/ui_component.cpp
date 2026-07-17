#include <av_root/ui_component.hpp>
#include <av_root/im_scope.hpp>

namespace avR
{
    UiComponent::UiComponent(std::string id) : root(id), id(std::move(id))
    {
    }

    void UiComponent::draw()
    {
        // Every node draws inside its own ID scope; render() is free to emit
        // widgets with any label without risking collisions with siblings.
        ScopedId idScope(this);
        render();
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
