#include <av_root/ui_container_component.hpp>

namespace avR
{
    UiContainerComponent::UiContainerComponent(std::string label) : UiComponent(label),
                                                                    label(std::move(label))
    {
    }

    UiContainerComponent::~UiContainerComponent()
    {
    }
} // namespace avR
