#pragma once
#include <av_root/ui_component.hpp>

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
    };
} // namespace avUi
