#pragma once

#include <av_root/ui_component.hpp>
#include <av_root/av_state.hpp>
namespace avR
{
	class UiStatefullComponent : public UiComponent
    {
    public:
        UiStatefullComponent() = delete;
        virtual ~UiStatefullComponent();
        explicit UiStatefullComponent(std::shared_ptr<AvState> statePtr,std::string id);

        AvState* get_state() const { return statePtr.get(); }

    private:
        std::shared_ptr<AvState> statePtr;
    };
} // namespace avR
