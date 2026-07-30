#pragma once

#include <av_root/ui_component.hpp>
#include <av_root/av_inter_view_shared_state.hpp>

namespace avUi
{
    class SearchViewUi : public avR::UiComponent
    {
    public:
        SearchViewUi() = delete;
        ~SearchViewUi();

        explicit SearchViewUi(std::string id, avR::AvState *sharedState);

    private:
        avR::AvInterViewSharedState *shared_state;

        void render() override;
    };
} // namespace avUi
