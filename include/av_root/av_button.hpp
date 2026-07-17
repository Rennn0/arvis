#pragma once

#include <string>
#include <av_root/ui_component.hpp>

namespace avR
{
    class AvButton : public UiComponent
    {
    public:
        explicit AvButton(std::string label, Style style);
        ~AvButton();

        ImVec2 preferred_size() const override;

    protected:
        void render() override;

    private:
        std::string label;
        Style style;
    };
} // namespace avR
