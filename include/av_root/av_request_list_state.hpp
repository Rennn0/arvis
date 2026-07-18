#pragma once
#include <vector>
#include <av_root/av_state.hpp>
#include <av_root/av_request.hpp>
namespace avR
{
    class AvRequestListState : public AvState
    {
    public:
        std::vector<AvRequest> requests;
        std::string environment;

    public:
        AvRequestListState();
        ~AvRequestListState();

    private:
    };
} // namespace avR
