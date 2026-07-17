#pragma once
#include <av_root/ui_statefull_component.hpp>
#include <av_root/av_request.hpp>

namespace avUi
{
    class DetailedRequestViewUi : public avR::UiStatefullComponent
    {
    public:
        explicit DetailedRequestViewUi(std::shared_ptr<avR::AvState> statePtr, std::string id);
        DetailedRequestViewUi() = delete;
        ~DetailedRequestViewUi();

    private:
        void render() override;
    };
} // namespace avUi
