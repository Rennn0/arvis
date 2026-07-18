#pragma once

#include <av_root/av_state.hpp>
#include <av_root/av_request.hpp>
namespace avR
{
    class AvInterViewSharedState : public AvState
    {
    public:
        bool show_req_list_view;
        bool show_req_detailed_view;
        const AvRequest *display_request;

    public:
        AvInterViewSharedState();
        ~AvInterViewSharedState();

    private:
    };
} // namespace avR
