#include <av_root/ui_statefull_component.hpp>

namespace avR
{
    UiStatefullComponent::~UiStatefullComponent()
    {
    }

	UiStatefullComponent::UiStatefullComponent(std::shared_ptr<AvState> statePtr, std::string id) 
        : UiComponent(std::move(id)), statePtr(std::move(statePtr))
    {
    }
} // namespace avR
