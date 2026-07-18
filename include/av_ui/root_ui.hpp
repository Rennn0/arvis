#pragma once
#include <av_root/ui_component.hpp>
#include <av_root/av_state.hpp>

namespace avUi
{
    class RootUi : public avR::UiComponent
    {
    public:
        explicit RootUi(std::string);
        ~RootUi();

    private:
        void render() override;

        const ImGuiViewport *viewport;
        std::shared_ptr<avR::AvState> inter_view_state;
    };
} // namespace avUi
