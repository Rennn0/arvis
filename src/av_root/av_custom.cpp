#include <av_root/av_custom.hpp>

namespace avR
{
    AvCustom::AvCustom(std::string id, DrawFn drawFn)
        : UiComponent(std::move(id)), drawFn(std::move(drawFn))
    {
    }

    void AvCustom::render()
    {
        // ID scope is opened by UiComponent::draw(); just run the callback.
        if (this->drawFn)
            this->drawFn();
    }
} // namespace avR
