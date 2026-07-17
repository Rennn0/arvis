#pragma once
#include <av_root/av_request.hpp>
#include <av_root/ui_component.hpp>

namespace avUi
{
    class DetailedRequestViewUi : public avR::UiComponent
    {
    public:
        explicit DetailedRequestViewUi(std::string id);
        ~DetailedRequestViewUi();

    private:
        void render() override;
    };
} // namespace avUi
